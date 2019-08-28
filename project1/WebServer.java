import java.io.* ;
import java.net.* ;
import java.util.* ;

public final class WebServer {
	// Set the port number.
	private final static int port = 6789;
	public static void main(String[] args) throws Exception {		
		// Establish the listen socket.
		/* Fill in */
		
		// Process HTTP service requests in an infinite loop.
		while (true) {
			// Listen for a TCP connection request.
			/* Fill in */
			// Construct an object to process the HTTP request message.
			HttpRequestRunnable request = new HttpRequestRunnable( /* Fill in */ );

			// Create a new thread to process the request.
			Thread thread = new Thread(request);

			// Start the thread.
			thread.start();
		}
	}
}

final class HttpRequestRunnable implements Runnable
{
	final static String CRLF = "\r\n";

	private Socket clientSocket;

	HttpRequestRunnable(Socket socket) {
		clientSocket = socket;
	}
	@Override
	public void run() {
		try {
			processRequest();
		} catch (Exception e) {
			System.out.println(e);
		}		
	}
	private void processRequest() throws Exception {
		// Get a reference to the socket's input and output streams.
		InputStream is = /* Fill in */;
		DataOutputStream os = /* Fill in */;

		// Set up input stream filters.
		/* Fill in */
		BufferedReader br = /* Fill in */;

		// Get the request line of the HTTP request message.
		String requestLine = /* Fill in */;

		// Display the request line.
		System.out.println();
		System.out.println(requestLine);

		// Get and display the header lines.
		String headerLine = null;
		while ((headerLine = br.readLine()).length() != 0) {
			System.out.println(headerLine);
		}

		// Extract the filename from the request line.
		StringTokenizer tokens = new StringTokenizer(requestLine);
		tokens.nextToken();  // skip over the method, which should be "GET"
		String fileName = tokens.nextToken();

		// Prepend a "." so that file request is within the current directory.
		fileName = "." + fileName;
		// Open the requested file.
		FileInputStream fis = null;
		boolean fileExists = true;
		try {
			fis = new FileInputStream(fileName);
		} catch (FileNotFoundException e) {
			fileExists = false;
		}

		// Construct the response message.
		String statusLine = null;
		String contentTypeLine = null;
		String entityBody = null;
		if (fileExists) {
			statusLine = /* Fill in */;
			contentTypeLine = "Content-type: " + 
					contentType( fileName ) + CRLF;
		} else {
			statusLine = /* Fill in */;
			contentTypeLine = /* Fill in */;
			entityBody = "<HTML>" + 
					"<HEAD><TITLE>Not Found</TITLE></HEAD>" +
					"<BODY>Not Found</BODY></HTML>";
		}

		// Send the status line.
		os.writeBytes(statusLine);

		// Send the content type line.
		os.writeBytes(/* Fill in */);

		// Send a blank line to indicate the end of the header lines.
		os.writeBytes(CRLF);

		// Send the entity body.
		if (fileExists)	{
			sendBytes(fis, os);
			fis.close();
		} else {
			os.writeBytes(/* Fill in */);
		}
		// Close streams and socket.
		os.close();
		br.close();
		clientSocket.close();

	}

	private static void sendBytes(FileInputStream fis, OutputStream os) 
			throws Exception
	{
		// Construct a 1K buffer to hold bytes on their way to the socket.
		byte[] buffer = new byte[1024];
		int bytes = 0;

		// Copy requested file into the socket's output stream.
		while((bytes = fis.read(buffer)) != -1 ) {
			os.write(buffer, 0, bytes);
		}
	}

	private static String contentType(String fileName)
	{
		if(fileName.endsWith(".htm") || fileName.endsWith(".html")) {
			return "text/html";
		}
		if(/* Fill in */) {
			return /* Fill in */;
		}
		if(/* Fill in */) {
			return /* Fill in */;
		}
		return "application/octet-stream";
	}

}
