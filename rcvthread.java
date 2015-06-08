
import java.io.IOException;
import java.net.Socket;

public class rcvthread implements Runnable {

	private logger logger;
	private final int sizeBuf = 50;
	private int flag;
	private Socket socket;
	private String rcvData = "Error";
	private byte[] rcvBuf = new byte[sizeBuf];
	private int rcvBufSize;
	
	public rcvthread(logger logger, Socket socket){
		this.logger = logger;
		flag = 1;
		this.socket = socket;
	}

	public void setFlag(int setflag) {
		flag = setflag;
	}
	
	public void run() {
		while(flag == 1){		
			try{	
				rcvBufSize = socket.getInputStream().read(rcvBuf);
				rcvData = new String(rcvBuf, 0, rcvBufSize, "UTF-8");
				
				if (rcvData.compareTo("[close]")==0){
					flag = 0;
				}
				logger.log("Recive Data : " + rcvData);
			} catch (IOException e){
				e.printStackTrace();
			}
		}
		logger.log("Exit loop");
	}		
}

