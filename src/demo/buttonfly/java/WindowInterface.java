//  SyzygyInterface.java

import java.awt.*;
import java.awt.event.*;
import java.util.*;

class ButtonInterface extends Frame implements ActionListener {
  String subnet_;
  Label lblInterfaceName_ = null;
  TextField tfInterface_ = null;
  Button btnConnect_ = null;

  SyzygyInterface interface_ = null;
  LightClient lights_ = new LightClient("atomgate.isl.uiuc.edu");
  int citem = 0;
  String[] szItem = null;
  boolean connected_ = false;
  boolean commandBeingProcessed = false;

  // this class handles the asynchronous processing of the button
  // presses. this allows us to avoid letting user interface events
  // get queued up during the wait for the handshake back
  class CommandHandlerThread extends Thread {
    ButtonInterface myInterface = null;
    int i = 0;

    public void setInterface(ButtonInterface theInterface){
      myInterface = theInterface;
    }

    public void setMessage(int messageInt){
      i = messageInt;
    }

    public void run(){
      if (myInterface == null){
	return;
      }
      // NOTE: these various colors ARE NOT IMPLEMENTED YET!!!
      // (i.e. no handshake occurs! we'd need sequence numbers for
      //  that to work)
      int state = myInterface.interface_.sendButton(i);
      if (state == 0){
	myInterface.setBackground(Color.blue);
      }
      else if (state == 1){
	myInterface.setBackground(Color.black);
      }
      else if (state == 2){
	myInterface.setBackground(Color.lightGray);
      }
      else{
        // this should not occur
	myInterface.setBackground(Color.yellow);
      }
      myInterface.commandBeingProcessed = false;
    }
  }

  ButtonInterface(String theNet) {
    super("ButtonInterface");
    subnet_ = theNet;
    setSize(240,100);
    // FlowLayout's textfield widths are buggy on ipaq
    setLayout(new BorderLayout());
    addWindowListener(new BasicWindowMonitor());

    // First present a dialog to enter the service tag.

    lblInterfaceName_ = new Label("Enter interface:");
    tfInterface_ = new TextField("");
    btnConnect_ = new Button("Connect");
    btnConnect_.addActionListener(this);

    add(lblInterfaceName_, BorderLayout.NORTH);
    add(tfInterface_, BorderLayout.CENTER);
    add(btnConnect_, BorderLayout.SOUTH);
    validate();
    show();
  }

  // This gets notified when the "Connect" button is pressed.
  public void actionPerformed(ActionEvent e) {
    String str = e.getActionCommand();
    // we must ignore the event if a command is currently being processed
    if (commandBeingProcessed){
      return;
    }

    // Initial dialog.

    if (str.equals("Connect")) {
      interface_ = new SyzygyInterface();
      if (interface_.connect(tfInterface_.getText())) {
	// Need to get the button info
	citem = interface_.numberButtons;
	szItem = interface_.buttonNames;
	// Remove the dialog.
	lblInterfaceName_.setText("Connected.");
	remove(tfInterface_);
	remove(btnConnect_);
	validate();
	show();
	remove(lblInterfaceName_);
	mainScreen();
      }
      else {
	lblInterfaceName_.setText("Failed.   Enter new host name");
      }
    }

    // Second screen:  buttons and stuff.

    // light functions:
    if (str.equals("Lights On"))
      lights_.selectScene(1);
    else if (str.equals("Lights Off"))
      lights_.selectScene(2);

    // command functions:
    else
      for (int i=0; i<citem; i++) {
	if (str.equals(szItem[i])) {
	  // we use a helper thread so that we can reject input events
	  // that occur while the application is starting
	  commandBeingProcessed = true;
	  CommandHandlerThread helper = new CommandHandlerThread();
          helper.setInterface(this);
          helper.setMessage(i);
          helper.setPriority(10);
          helper.start();
	  break;
	}
      }

    Toolkit.getDefaultToolkit().beep();
  }
  
  void mainScreen() {
    // interface_ must be connected to syzygy!
    setSize(240,320);
    setLayout(new FlowLayout());
    Thread watchdog_;

    for (int i=0; i<citem; i++) {
      addButton(szItem[i]);
    }

    addButton("Lights On");
    addButton("Lights Off");
    validate();

    connected_ = true;

    // show this window
    show();
  }

  void addButton(String name) {
    Button b = new Button(name);
    add(b);
    b.addActionListener(this);
  }
}

// Simply the main class to get things going.
class WindowInterface {
  public static void main (String args[]) {
    String subnet = "255.255.255";
    if (args.length >= 1)
      subnet = args[0];
    ButtonInterface i = new ButtonInterface(subnet);
    i.setVisible(true);
  }
}
