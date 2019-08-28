/**
 * ProxyCache.java - Simple caching proxy
 *
 * $Id: ProxyCache.java,v 1.3 2004/02/16 15:22:00 kangasha Exp $
 *
 */

import java.net.*;
import java.io.*;
import java.util.*;

public class ProxyCache {
	/** Port for the proxy */
	private static int port;
	/** Socket for client connections */
	private static ServerSocket socket;

	/** Create the ProxyCache object and the socket */
	public static void init(int p) {
		port = p;
		try {
			socket = /* Fill in */;
		} catch (IOException e) {
			System.out.println("Error creating socket: " + e);
			System.exit(-1);
		}
	}

	/** Read command line arguments and start proxy */
	public static void main(String args[]) {
		int myPort = 0;

		try {
			myPort = Integer.parseInt(args[0]);
		} catch (ArrayIndexOutOfBoundsException e) {
			System.out.println("Need port number as argument");
			System.exit(-1);
		} catch (NumberFormatException e) {
			System.out.println("Please give port number as integer.");
			System.exit(-1);
		}

		init(myPort);

		/** Main loop. Listen for incoming connections and spawn a new
		 * thread for handling them */
		Socket client = null;

		while (true) {
			try {
				client = /* Fill in */;
				ProxyRunnable pr = new ProxyRunnable(client);
				Thread thread = new Thread(pr);
				thread.start();
			} catch (IOException e) {
				System.out.println("Error reading request from client: " + e);
				/* Definitely cannot continue processing this request,
				 * so skip to next iteration of while loop. */
				continue;
			}
		}

	}
}

final class ProxyRunnable implements Runnable {
	private Socket client;

	ProxyRunnable(Socket client) {
		this.client = client;
	}
	@Override
	public void run() {
		/* Fill in */;		
	}

	private void handle() {
		Socket server = null;
		HttpRequest request = null;
		HttpResponse response = null;

		/* Process request. If there are any exceptions, then simply
		 * return and end this request. This unfortunately means the
		 * client will hang for a while, until it timeouts. */

		/* Read request */
		try {
			DataInputStream fromClient = /* Fill in */;
			request = /* Fill in */;
		} catch (IOException e) {
			System.out.println("Error reading request from client: " + e);
			return;
		}
		/* Fill in */
		/* Serve the requested object from cache
		 * if has been previously cached.
		 */
		
		/* Send request to server */
		try {
			/* Open socket and write request to socket */
			server = /* Fill in */;
			DataOutputStream toServer = /* Fill in */;

			/* Fill in */
			if(request.isPost) {
				/* Fill in */;
			}
		} catch (UnknownHostException e) {
			System.out.println("Unknown host: " + request.getHost());
			System.out.println(e);
			return;
		} catch (IOException e) {
			System.out.println("Error writing request to server: " + e);
			return;
		}
		/* Read response and forward it to client */
		try {
			DataInputStream fromServer = /* Fill in */;
			response = /* Fill in */;
			DataOutputStream toClient = /* Fill in */;
		    /* Write response to client. First headers, then body */
			toClient.writeBytes(/* Fill in */);
			/* Fill in */
			client.close();
			server.close();
			/* Insert object into the cache */
			/* Fill in */
		} catch (IOException e) {
			System.out.println("Error writing response to client: " + e);
		}

	}
}
