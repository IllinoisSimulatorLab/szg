//  SyzygyInterface.java

import java.io.*;
import java.net.*;

// Discover a syzygy service "JavaInterface" and send integers to it.

public class SyzygyInterface {
  public String[] buttonNames = null;
  public int numberButtons = 0;
  // TCP socket connetion to the service
  private Socket socket_ = null;
  private byte[] buf = new byte[4];
  DatagramSocket discoverySocket = null;
  InetAddress broadcast = null;

  SyzygyInterface(){
    try {
      // create the discovery socket and packet
      discoverySocket = new DatagramSocket(4623);
    }
    catch(SocketException e){
      System.out.println("SyzygyInterface error: failed to create socket.");
      System.exit(1);
    }
  }

  public int parseInt(byte[] data, int offset){
    return ( data[offset]        & 0xFF) |
	   ((data[offset+1] >>  8) & 0xFF00) |
	   ((data[offset+2] >> 16) & 0xFF0000) |
	   ((data[offset+3] >> 24) & 0xFF000000);
  }

  public void packInt(int data, byte[] buffer, int offset){
    buffer[offset] = (byte)(data & 0xFF);
    buffer[offset+1] = (byte)((data & 0xFF00) >> 8);
    buffer[offset+2] = (byte)((data & 0xFF0000) >> 16);
    buffer[offset+3] = (byte)((data & 0xFF000000) >> 24);
  }

  // Send an integer to the service (4 bytes)
  public boolean sendInt(int val) {
    if (socket_ == null) {
      System.err.println("SyzygyInterface error: not connected.");
      return false;
    }

    try {
      // split the int into bytes:
      buf[0] = (byte)(val & 0xFF);
      buf[1] = (byte)((val & 0xFF00) >> 8);
      buf[2] = (byte)((val & 0xFF0000) >> 16);
      buf[3] = (byte)((val & 0xFF000000) >> 24);

      // send the data:
      OutputStream out = socket_.getOutputStream();
      out.write(buf,0,4);
      out.flush();
    }
    catch(IOException ev) {
      System.out.println("SyzygyInterface error: sendInt failed to write to socket.");
      return false;
    }
    return true;
  }

  // Read an integer from the service (4 bytes)
  public int recvInt() {
    if (socket_ == null) {
      System.err.println("SyzygyInterface error: not connected.");
      return -1;
    }

    try {
      // read the data:
      InputStream in = socket_.getInputStream();
      in.read(buf,0,4);
      // assemble the bytes into an int.
      return ( buf[0]        & 0xFF) |
	     ((buf[1] >>  8) & 0xFF00) |
	     ((buf[2] >> 16) & 0xFF0000) |
	     ((buf[3] >> 24) & 0xFF000000);
    }
    catch(IOException ev) {
      System.out.println("SyzygyInterface error: recvInt failed to read from socket.");
      return -1;
    }
  }

  public boolean recvHandshake(){
    if (socket_ == null){
      System.err.println("SyzygyInterface error: not connected.");
      return false;
    }
    try {
      byte[] myBuf = new byte[4];
      InputStream in = socket_.getInputStream();
      in.read(myBuf,0,4);
      // we don't really need to do anything here... we're
      // just receiving the handshake... not acting on the contents
      return true;
    }
    catch(IOException ev){
      System.out.println("SyzygyInterface error: handshake failed to read from socket.");
      return false;
    }
  }

  // Read an integer from the service (4 bytes)
  public String recvString() {
    if (socket_ == null) {
      System.err.println("SyzygyInterface error: not connected.");
      return "";
    }

    try {
      // read the data:
      InputStream in = socket_.getInputStream();
      in.read(buf,0,4); // length of string
      int len = ( buf[0]        & 0xFF) |
	        ((buf[1] >>  8) & 0xFF00) |
	        ((buf[2] >> 16) & 0xFF0000) |
	        ((buf[3] >> 24) & 0xFF000000);
      byte[] sz = new byte[len];
      in.read(sz,0,len);
      return new String(sz);
    }
    catch(IOException ev) {
      System.out.println("SyzygyInterface error: recvString failed to read from socket.");
      return "";
    }
  }

  // Disconnect from the service.
  public boolean disconnect() {
    try {
      if (socket_ != null)
	socket_.close();
      socket_ = null;
    }
    catch (java.io.IOException e) {
      System.err.println("SyzygyInterface error: failed to close socket.");
      return false;
    }
    return true;
  }

  // Parses the interface packet into internal storage
  public void parseInterfacePacket(byte[] dataField){
    numberButtons = parseInt(dataField,0);
    buttonNames = new String[numberButtons];
    System.out.print(numberButtons);
    int where = 4;
    for (int i=0; i<numberButtons; i++){
      int stringLength = parseInt(dataField, where);
      where += 4;
      buttonNames[i] = new String(dataField, where, stringLength);
      where += stringLength;
    }
  }

  public int sendButton(int buttonID){
    // data component to the UDP packets
    byte[] dataField = new byte[1024];
    // we are making an button request
    String packetType = "button";
    // pack the dataField
    byte[] typeBytes = packetType.getBytes();
    for (int i=0; i<packetType.length(); i++){
      dataField[i] = typeBytes[i];
    }
    dataField[packetType.length()] = 0;
    // pack the button value
    packInt(buttonID, dataField, 128);
    
    DatagramPacket discoveryPacket =
      new DatagramPacket(dataField,1024,broadcast,4622);
    try{
      discoverySocket.send(discoveryPacket);
    }
    catch (IOException e){
      System.out.println("SyzygyInterface error: send button failed.\n");
    }

    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // We really want to make this handshake with the ButtonInterface
    // program... but to do that we need to do packet sequence numbers,
    // etc... that's for another time.
    
    return 2;
  }

  // Discover and connect to the service "JavaInterface"
  // with the service tag specified.
  public boolean connect(String destination) {

    // data component to the UDP packets
    byte[] dataField = new byte[1024];
    // we are making an interface request
    String packetType = "interface";
    // pack the dataField
    byte[] typeBytes = packetType.getBytes();
    for (int i=0; i<packetType.length(); i++){
      dataField[i] = typeBytes[i];
    }
    dataField[packetType.length()] = 0;


    try {
      broadcast = InetAddress.getByName(destination);
      DatagramPacket discoveryPacket =
	new DatagramPacket(dataField,1024,broadcast,4622);

      // send the interface request packet
      try {
	discoverySocket.send(discoveryPacket);
      }
      catch(IOException e) {
	System.out.println("SyzygyInterface error: interface request failed.");
	return false;
      }

      // wait for a response
      try {
	// a long time-out is desirable, in this case 5 seconds
	discoverySocket.setSoTimeout(5000);
	try {
	  discoverySocket.receive(discoveryPacket);
	}
	catch(InterruptedIOException e) {
	  discoverySocket.close();
	  return false;
	}

        // figure out what's in the packet
        parseInterfacePacket(dataField);
      }
      catch(IOException e) {
	System.out.println("SyzygyInterface error: interface receive failed.");
	return false;
      }
    }
    catch(UnknownHostException e) {
      System.out.println("SyzygyInterface error: interface request failed.");
      return false;
    }

    return true;
  }
}
