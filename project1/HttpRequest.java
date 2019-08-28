/**
 * HttpRequest - HTTP request container and parser
 *
 * $Id: HttpRequest.java,v 1.2 2003/11/26 18:11:53 kangasha Exp $
 *
 */

import java.io.*;
import java.net.*;
import java.util.*;

public class HttpRequest {
	/** Help variables */
	final static String CRLF = "\r\n";
	final static int HTTP_PORT = 80;
	/** How big is the buffer used for reading the object */
	final static int BUF_SIZE = 8192;
	/** Maximum size of objects that this proxy can handle. For the
	 * moment set to 100 KB. You can adjust this as needed. */
	final static int MAX_OBJECT_SIZE = 100000;
	/** Store the request parameters */
	String method;
	String URI;
	String version;
	String headers = "";
	/** Server and port */
	private String host;
	private int port;
	/* Body of request */
	byte[] body = new byte[MAX_OBJECT_SIZE];
	boolean isPost = false;
	private int length; 


	/** Create HttpRequest by reading it from the client socket */
	@SuppressWarnings("deprecation")
	public HttpRequest(DataInputStream from) {
		String firstLine = "";
		try {
			firstLine = from.readLine();
		} catch (IOException e) {
			System.out.println("Error reading request line: " + e);
		}

		System.out.println(firstLine);

		String[] tmp = firstLine.split(" ");
		method = /* Fill in */;
		URI = /* Fill in */;
		version =/* Fill in */;

		System.out.println("URI is: " + URI);

		if ((!method.equals("GET")) && (!method.equals("POST"))) {
			System.out.println("Error: Method not GET nor POST");
		}

		if(method.equals("POST")) {
			isPost = true;
		}
		try {
			String line = from.readLine();
			while (line.length() != 0) {
				headers += line + CRLF;
				/* We need to find host header to know which server to
				 * contact in case the request URI is not complete. */
				if (line.startsWith("Host:")) {
					tmp = line.split(" ");
					// host:port?
					if (tmp[1].indexOf(':') > 0) {
						String[] tmp2 = tmp[1].split(":");
						host = tmp2[0];
						port = Integer.parseInt(tmp2[1]);
					} else {
						host = tmp[1];
						port = HTTP_PORT;
					}
				}
				/* Get length of content as indicated by
				 * Content-Length header. Unfortunately this is not
				 * present in every response. Some servers return the
				 * header "Content-Length", others return
				 * "Content-length". You need to check for both
				 * here. */
				if (line.startsWith(/* Fill in */) ||
						line.startsWith(/* Fill in */)) {
					tmp = line.split(" ");
					length = Integer.parseInt(tmp[1]);
				}
				line = from.readLine();
			}
		} catch (IOException e) {
			System.out.println("Error reading from socket: " + e);
			return;
		}
		
		System.out.println("Request Headers: " + headers);


		if(isPost) {
			//Read request body
			try {
				int bytesRead = 0;
				byte buf[] = new byte[BUF_SIZE];
				boolean loop = false;

				/* If we didn't get Content-Length header, just loop until
				 * the connection is closed. */
				if (length == -1) {
					loop = true;
				}

				/* Read the body in chunks of BUF_SIZE and copy the chunk
				 * into body. Usually replies come back in smaller chunks
				 * than BUF_SIZE. The while-loop ends when the connection is
				 * closed 
				 */
				while (bytesRead < length || loop) {
					/* Read it in as binary data */
					int res = /* Fill in */;
					if (res == -1) {
						break;
					}
					/* Copy the bytes into body. Make sure we don't exceed
					 * the maximum object size. */
					for (int i = 0; 
							i < res && (i + bytesRead) < MAX_OBJECT_SIZE; 
							i++) {
						/* Fill in */;
					}
					bytesRead += res;
				}
			} catch (IOException e) {
				System.out.println("Error reading response body: " + e);
				return;
			}
		}
		System.out.println("Host to contact is: " + host + " at port " + port);
	}

	/** Return host for which this request is intended */
	public String getHost() {
		return host;
	}

	/** Return port for server */
	public int getPort() {
		return port;
	}

	/**
	 * Convert request into a string for easy re-sending.
	 */
	public String toString() {
		String req = "";

		req = method + " " + URI + " " + version + CRLF;
		req += headers;
		/* This proxy does not support persistent connections */
		req += "Connection: close" + CRLF;
		req += CRLF;

		return req;
	}
}
