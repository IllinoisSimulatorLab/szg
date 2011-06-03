import sys
from getch import getch
from szg import arSZGClient, arAppLauncher

szgClient = arSZGClient()

def getMasterID( virtComp ):
  # The arAppLauncher can find out about virtual
  # computer parameters.
  launcher = arAppLauncher( 'keymsg.py' )
  launcher.setSZGClient( szgClient )

  # Set the desired virtual computer name
  if not launcher.setVircomp( virtComp ):
    print 'Failed to set virtual computer'
    sys.exit(1)

  # Misnomer, setParameters() loads parameters from
  # the server.
  if not launcher.setParameters():
    print 'Invalid virtual computer definition'
    sys.exit(1)

  lockName = launcher.getMasterName()
  gotLock, ownerID = szgClient.getLock( lockName )
  if gotLock:
    # nobody was holding the lock, i.e. there's no
    # master running on this virtual computer.
    szgClient.releaseLock( lockName )
    print 'No master running on virtual computer %s.' \
        % virtComp
    sys.exit(1)
  
  return ownerID


def eventLoop( masterID ):
  print 'Press <ESC> to exit'
  messageType = 'key'
  while True:
    # Get a keypress
    messageBody = getch()
    if ord( messageBody ) == 27: # ESC
      return
    print messageBody
    
    # Send it to the master
    match = szgClient.sendMessage( messageType, messageBody, \
        masterID, False )

    # If an error occurred, quit.
    if match < 0:
      return

  
def main():
  if len( sys.argv ) != 2:
    print "Usage: python keymsg.py <virtual computer>"
    sys.exit(0)

  virtualComputer = sys.argv[1]

  if not szgClient.init( sys.argv ):
    print 'Failed to initialize arSZGClient'
    sys.exit(1)

  masterID = getMasterID( virtualComputer )
  print 'Master ID = %d' % masterID

  eventLoop( masterID )

  sys.exit(0)

if __name__ == '__main__':
  main()
