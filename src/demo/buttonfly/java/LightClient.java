import java.io.*;
import java.net.*;

// Send little UDP packets to a server which controls Lutron room lighting.

public class LightClient
{
    String host_;
    DatagramSocket socket_;

    LightClient(String host)
    {
        host_ = host;
        try
        {
            socket_ = new DatagramSocket(5624);
        }
        catch (SocketException e)
        {
            System.err.println("cannot open socket.");
        }
    }

    // For now, scene 1 is all-on, scene 2 is all-off.
    boolean selectScene(int scene)
    {
        try
        {
            byte[] data = new byte[16];
            InetAddress dest = InetAddress.getByName(host_);
            DatagramPacket packet =
                new DatagramPacket(data, 16, dest, 5624);
            data[0] = (byte) scene;
            socket_.send(packet);
            return true;
        }
        catch (Exception e)
        {
            return false;
        }
    }
}
