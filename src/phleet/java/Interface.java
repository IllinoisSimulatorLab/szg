import java.awt.*;
import java.awt.event.*;
import java.net.*;
import java.io.*;
//import java.util.*;
//import java.lang.Exception;

class ReadThread implements Runnable{
    private DataInputStream _inStream;
    String[][] _names;
    int[][] _ID;
    List master;
    List slave;

    public void render(int which){
        master.removeAll();
	slave.removeAll();
	// **** kludge
	if (which < 30){
	    int i = 0;
	    while (_names[i][0]!=null && i<30){
		master.add(_names[i][0]);
		i++;
	    }
	    i = 1;
	    while (_names[which][i]!=null && i<30){
		slave.add(_names[which][i]);
		i++;
	    }
	}
    }

    ReadThread(DataInputStream inStream,List m, List s,String[][] names,
               int[][] id){
	master = m;
	slave = s;
	_inStream = inStream;
        _names = names;
        _ID = id;
    }

    public void appendTokens(String tok1, String tok2, int proc){
	int i = 0;
	// ***** kludge
	while (_names[i][0] != null && !_names[i][0].equals(tok1) && i<30){
	    i++;
	}
	if (i<30){
	    _names[i][0] = tok1;
	    int j = 1;
            while(_names[i][j]!=null && j<30){
		j++;
	    }
	    if (j<30){
		_names[i][j] = tok2;
                _ID[i][j] = proc;
	    }
	}
    }

    public void clearTokens(){
	// ****** AARGH some kludge action here
	for (int i=0; i<30; i++){
	    for (int j=0; j<30; j++){
	      _names[i][j] = null;
              _ID[i][j] = -1;
	    }
        }
    }

    public void run(){
        byte[] theBuffer = new byte[10000];
        // System.out.println("Starting the read thread\n");
	while(true){
	  try{
	      int theCode = _inStream.readInt();
              // System.out.println(theCode);
              int theLength = _inStream.readInt();
              // System.out.println(theLength);
              _inStream.read(theBuffer,0,theLength);
              // System.out.println("Got something back\n");
              byte[] stringBuf = new byte[theLength];
	      for (int i=0; i<theLength; i++){
		  stringBuf[i] = theBuffer[i];
	      }
	      String text = new String(stringBuf);
              clearTokens();
	      // System.out.println(text);
              boolean done = false;
	      String textPiece = text;
	      String printPiece = null;
              int lastPos = 0;
	      while(!done && lastPos<text.length()){
		  int pos = textPiece.indexOf(':')+lastPos;
		  if (pos==-1){
		      done = true;
		  }
		  else{
		      printPiece = text.substring(lastPos, pos+1);
                      textPiece = 
                        text.substring(pos+1,text.length());
		      lastPos = pos+1;
		      int divider = printPiece.indexOf('/');
		      String token1 = printPiece.substring(0,divider);
		      String otherHalf = 
			  printPiece.substring(divider+1,printPiece.length());
		      divider = otherHalf.indexOf('/');
                      String token2 = 
                          otherHalf.substring(0,divider);
                      Integer newInt = 
                        new Integer(otherHalf.substring(divider+1,
                                                        otherHalf.length()-1));
                      int process = newInt.intValue();
		      appendTokens(token1,token2,process);
		      // System.out.println(printPiece);
		  } 
	      }
	  }
	  catch(IOException e){
	      System.out.println("Failed to read from socket\n");
	      System.exit(1);
	  }
          // render lists
          render(0);
          master.select(0);
          // System.out.println("Got a socket packet");
	}
    }
}

class TextListener implements ActionListener{
  private String[] _output;
  private Frame _frame;
  public TextListener(Frame frame,String[] output){
    _frame = frame;
    _output = output;
  }
  public void actionPerformed(ActionEvent e){
    String s = e.getActionCommand();
    _frame.setVisible(false);
    _output[0] = s;
  }
}

class ButtonListener implements ActionListener{
  private DataOutputStream _outStream;
    private String[] _type;
    public ButtonListener(DataOutputStream out, String[] type){
	_outStream = out;
	_type = type;
    }
    public void actionPerformed(ActionEvent e){
      String s = e.getActionCommand();
      if (s.equals("AR_EXEC_BUTTON")){
	// System.out.println("Execute a program");
        try{
	  int crap = 4; // code for requesting the process list
	  byte[] buf = new byte[4];
          buf[0] = (byte)(crap & 0xFF);
	  buf[1] = (byte)((crap & 0xFF00) >> 8);
	  buf[2] = (byte)((crap & 0xFF0000) >> 16);
	  buf[3] = (byte)((crap & 0xFF000000) >> 24);
          _outStream.write(buf,0,4);
          _type[0] = "exec";
	}
        catch(IOException ev){
	  System.out.println("Failed to write to socket\n");
	  System.exit(1);
	}
      }
      else if (s.equals("AR_LIST_BUTTON")){
        try{
	  int crap = 1; // code for requesting the process list
	  byte[] buf = new byte[4];
          buf[0] = (byte)(crap & 0xFF);
	  buf[1] = (byte)((crap & 0xFF00) >> 8);
	  buf[2] = (byte)((crap & 0xFF0000) >> 16);
	  buf[3] = (byte)((crap & 0xFF000000) >> 24);
          _outStream.write(buf,0,4);
          _type[0] = "kill";
	}
        catch(IOException ev){
	  System.out.println("Failed to write to socket\n");
	  System.exit(1);
	}
      }
    }
}

class MyListener implements ItemListener{
    private List master;
    private List slave;
    private String[][] names;
    private int[][] ID;
    private DataOutputStream _outStream;

    public MyListener(DataOutputStream out, List m, List s, String[][] n,
                      int[][] i){
	master = m;
	slave = s;
	names = n;
        ID = i;
        _outStream = out;
    }
    public void render(int which){
        master.removeAll();
	slave.removeAll();
	// **** kludge
	if (which < 30){
	    int i = 0;
	    while (names[i][0]!=null && i<30){
		master.add(names[i][0]);
		i++;
	    }
	    i = 1;
	    while (names[which][i]!=null && i<30){
		slave.add(names[which][i]);
		i++;
	    }
	}
    }
	public void itemStateChanged(ItemEvent e){
	  int which = master.getSelectedIndex();
          render(which);
	  master.select(which);
	}
}

class TheListener implements ActionListener{
    private List master;
    private List slave;
    private String[][] names;
    private int[][] ID;
    private DataOutputStream _outStream;
    private String[] _type;

    public TheListener(DataOutputStream out, List m, List s, String[][] n,
                      int[][] i, String[] type){
	master = m;
	slave = s;
	names = n;
        ID = i;
        _outStream = out;
	_type = type;
    }

    public void actionPerformed(ActionEvent e){
      int which = slave.getSelectedIndex();
      // System.out.println(ID[master.getSelectedIndex()][which+1]);
      // System.out.println(_type[0]);
      slave.select(which);
      if (_type[0]=="kill"){
        try{
          int crap = 3; // code for sending a kill command
	  byte[] buf = new byte[4];
          buf[0] = (byte)(crap & 0xFF);
	  buf[1] = (byte)((crap & 0xFF00) >> 8);
	  buf[2] = (byte)((crap & 0xFF0000) >> 16);
	  buf[3] = (byte)((crap & 0xFF000000) >> 24);
          _outStream.write(buf,0,4);
          crap = ID[master.getSelectedIndex()][which+1];
          buf = new byte[4];
          buf[0] = (byte)(crap & 0xFF);
	  buf[1] = (byte)((crap & 0xFF00) >> 8);
	  buf[2] = (byte)((crap & 0xFF0000) >> 16);
	  buf[3] = (byte)((crap & 0xFF000000) >> 24);
          _outStream.write(buf,0,4);
        }
        catch(IOException ev){
	  System.out.println("Socket write failed\n");
	  System.exit(1);
        }
      }
      else if (_type[0]=="exec"){
	  try{
            int crap = 6; // code for sending a kill command
	    byte[] buf = new byte[4];
            buf[0] = (byte)(crap & 0xFF);
	    buf[1] = (byte)((crap & 0xFF00) >> 8);
	    buf[2] = (byte)((crap & 0xFF0000) >> 16);
	    buf[3] = (byte)((crap & 0xFF000000) >> 24);
            _outStream.write(buf,0,4);
            String computerName = names[master.getSelectedIndex()][0];
            String computerProcess = names[master.getSelectedIndex()][which+1];
            // System.out.println(computerName);
            // System.out.println(computerProcess);
            byte[] buffer = new byte[512];
            for (int i=0; i<computerName.length(); i++){
		buffer[i] = (byte) computerName.charAt(i);
	    }
            buffer[computerName.length()]= (byte) '/';
            for (int i=0; i<computerProcess.length(); i++){
		buffer[i+computerName.length()+1] = 
                  (byte)computerProcess.charAt(i);
	    }
            crap = computerName.length()+computerProcess.length()+1;
            buf[0] = (byte)(crap & 0xFF);
	    buf[1] = (byte)((crap & 0xFF00) >> 8);
	    buf[2] = (byte)((crap & 0xFF0000) >> 16);
	    buf[3] = (byte)((crap & 0xFF000000) >> 24);
            _outStream.write(buf,0,4);
            _outStream.write(buffer,0,crap);
	  }
          catch(IOException ev){
	    System.out.println("Failed to write to socket\n");
	    System.exit(1);
          }
      }
    }
}

class InterfaceThread extends Thread{

    String subnet;

    public void setSubnet(String theNet){
	subnet = theNet;
    }

    public void run(){

      final Frame f = new Frame("Syzygy Remote Control");
      final Frame d = new Frame("Enter Interface Name");
      List masterList = new List(8); 
      List slaveList = new List(8);
      String[][] theNames = new String[30][30];
      int[][] theID = new int[30][30];
      String[] actionType = new String[1];
      actionType[0] = "junk";

      // discovery process whereby we figure out where the desired service is
      // running
      String host="000.000.000.000";
      String[] serviceTag = new String[1];
      serviceTag[0] = "junk";
      int port = -1;

      // first, need to prompt user for the service tag
      d.setSize(240,160);
      d.setLayout(new FlowLayout());
      TextField inputArea = new TextField(15);
      inputArea.addActionListener(new TextListener(d,serviceTag));
      d.add(inputArea);
      d.addWindowListener(new WindowAdapter(){
		    public void windowClosing(WindowEvent e){
		    d.setVisible(false);
		    d.dispose();
		}
	    });
      d.show();
      while (d.isVisible()){
	try{
          sleep(100);
	}
	catch(InterruptedException e){}
      }

      try{
          DatagramSocket discoverySocket = new DatagramSocket(4623);
          byte[] dataField = new byte[256];
          String serviceType = new String("JavaInterface");
          int i;
          for (i=0; i<serviceType.length(); i++){
	      dataField[i] = (byte) serviceType.charAt(i);
          }
          dataField[serviceType.length()] = 0;
          for (i=0; i<serviceTag[0].length(); i++){
	      dataField[i+128] = (byte) serviceTag[0].charAt(i);
          }
          dataField[serviceTag[0].length()+128] = 0;
          try{
              InetAddress broadcast = 
                InetAddress.getByName(subnet.concat(".255"));
              DatagramPacket discoveryPacket = 
                new DatagramPacket(dataField,256,broadcast,4622);
	      try{
                  discoverySocket.send(discoveryPacket);
	      }
	      catch(IOException e){
		System.out.println("Discovery send failed\n");
		System.exit(1);
	      }
              try{
                discoverySocket.setSoTimeout(1000);
                try{
		  discoverySocket.receive(discoveryPacket);
		}
		catch(InterruptedIOException e){
		  System.out.println("No JavaInterface with that tag\n");
		  System.exit(1);
		}
                // we want the IP address from which this packet emerged
                host = discoveryPacket.getAddress().getHostAddress();
                // find the first 0 in the received bytes, this is
		// the first non-digit
		int j = -1;
                for (i=0; i<16; i++){
		  if (dataField[i]=='\0' && j==-1){
                    j=i;
		  }
		}
                String interfacePort = new String(dataField,0,j);
                port = Integer.valueOf(interfacePort).intValue();
	      }
	      catch(IOException e){
		System.out.println("Discovery receive failed.\n");
		System.exit(1);
	      }
          }
          catch(UnknownHostException e){
	      System.out.println("No broadcast\n");
	      System.exit(1);
          }
      }
      catch(SocketException e){
	System.out.println("Could not create discovery socket\n");
	System.exit(1);
      }
      
      // now we actually do the connection to the remote machine

       // create socket connection

	Socket socket = null;
	DataOutputStream out=null;
	DataInputStream in=null;

        try{
	    socket = new Socket(host, port);
            out = new DataOutputStream(socket.getOutputStream());
	    in = new DataInputStream(socket.getInputStream()); 
	}
	catch(UnknownHostException e){
	    System.out.println("Unknown host " + host + ".\n");
	    System.exit(1);
	}
        catch(IOException e){
	    System.out.println("Failed to connect to JavaInterface program on "
	      + host + "/" + port + "\n");
	    System.exit(1);
	}

        // must start the receive thread here 
        ReadThread r = new ReadThread(in,masterList,slaveList,theNames,theID); 
        Thread t = new Thread(r);
        t.start();

        f.setSize(240,320);
        f.setLayout(new FlowLayout());
        Button execButton = new Button(" Execute ");
        f.add(execButton);
	Button listButton = new Button("   List   ");
	f.add(listButton);
        f.addWindowListener(new WindowAdapter(){
		public void windowClosing(WindowEvent e){
		    f.setVisible(false);
		    f.dispose();
                    //socket.close();
		    System.exit(0);
		}
	    });

        ItemListener a = new MyListener(out,masterList,
                                        slaveList,theNames,theID);
        ActionListener b =  new TheListener(out,masterList,
                                          slaveList,theNames,theID,
                                          actionType);
        ActionListener c = new ButtonListener(out,actionType);
        execButton.setActionCommand("AR_EXEC_BUTTON");
	listButton.setActionCommand("AR_LIST_BUTTON");
        execButton.addActionListener(c);
	listButton.addActionListener(c);
	masterList.addItemListener(a);
        slaveList.addActionListener(b);

	f.add(masterList);
	f.add(slaveList);

        f.show();
    }
}


public class Interface{
    public static void main(String[] args){
        String subnet = "255.255.255";
	if (args.length >= 1){
	    subnet = args[0];
	}
        InterfaceThread r = new InterfaceThread();
        r.setSubnet(subnet);
        Thread t = new Thread(r);
        t.start();
    }
}
