#!/usr/local/bin/python

import SocketServer
import sys

class MyTCPHandler(SocketServer.BaseRequestHandler):
    """
    The request handler class for our server.
    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    lient.
    """

    def handle(self):
        messageSize = 1024
        keepListening = True
        head = " # READING A FILE \n"
        lines = ""
        tail = ">> A Friendly message from the server\n"

        while keepListening:
            self.data = self.request.recv(messageSize)

            # If there was an error with the connection, stop listening
            if not self.data or self.data == "$die":
                print("client> Client requested closed connection")
                keepListening = False
                self.server.server_close()

            # If the message does not includes :file tag, then is
            # just a text message we need to echo
            elif ":file" not in self.data:
                self.data = self.data.strip()
                print("client> " + self.data)
                self.request.send(self.data.upper())

            # If the client sends this tag, then we will read a text file
            elif self.data == ":file":
                endOfFile = False

                # Reads from the socket until there is nothing else,
                # then, it stops the while loop and prints the file
                # to the screeen.
                while not endOfFile:
                    text = self.request.recv(messageSize).strip()

                    print(text)
                    if ":eof" in text:
                        endOfFile = True

                    lines = lines + text

                # Remove the control message from the text
                # and generate the final message to store it as
                # a text file
                lines = lines.replace(":file", "")
                lines = lines.replace(":eof", "")
                lines = lines + "\n" + tail

                # Print the content in the screen
                print(head + lines)

                outputFile = open("./tempFile.txt", "w")
                outputFile.write(lines)
                outputFile.close()

                # Read and send the file to the client
                outputFile = open("./tempFile.txt", "r")
                outputText = outputFile.readlines()
                for outText in outputText:
                    self.request.send(outText)

                # Close the file and send a message
                # to the client indicating OEF
                outputFile.close()
                self.request.send(":eof")


# Main cycle
if __name__ == "__main__":
    sys.tracebacklimit = None

    # TODO Change the port and ip
    HOST, PORT = "localhost", 9999
    serverStop = False

    # Create the server, binding to localhost on port 9999
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    try:
        server.serve_forever()
    except Exception as error:
        print("server> Server closed")
