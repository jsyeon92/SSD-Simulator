import android.os.Bundle;
import android.app.Activity;
import android.content.Context;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.TextView;
import android.widget.Button;
import android.widget.EditText;

import java.net.Socket;
import java.io.IOException;
import java.io.OutputStream;

public class teleopclient extends Activity implements OnClickListener{

	private EditText editTextIPAddress;
	private TextView textViewStatus;
	private Button buttonConnect;
	private Button buttonClose;
	private Button buttonUp;
	private Button buttonLeftTurn;
	private Button buttonRightTurn;
	private Button buttonDown;
	private Button buttonStop;
	private InputMethodManager imm;
	private String server = "192.168.43.63";
	private int port = 59807;
	private Socket socket;
	private OutputStream outs;
	private Thread rcvThread;
	public logger logger;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
			
		editTextIPAddress = (EditText)this.findViewById(R.id.editTextIPAddress);
		editTextIPAddress.setText(server);
		imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
				
		textViewStatus = (TextView)this.findViewById(R.id.textViewStatus);
		textViewStatus.setText("TeleOp Client");
		
		logger = new logger(textViewStatus);
		
		buttonConnect   = (Button)this.findViewById(R.id.buttonConnect);
		buttonClose     = (Button)this.findViewById(R.id.buttonClose);
		buttonUp        = (Button)this.findViewById(R.id.buttonUp);
		buttonLeftTurn  = (Button)this.findViewById(R.id.buttonLeftTurn);
		buttonRightTurn = (Button)this.findViewById(R.id.buttonRightTurn);
		buttonDown      = (Button)this.findViewById(R.id.buttonDown);
		buttonStop      = (Button)this.findViewById(R.id.buttonStop);
		
		buttonConnect.setOnClickListener(this);
		buttonClose.setOnClickListener(this);
		buttonUp.setOnClickListener(this);
		buttonLeftTurn.setOnClickListener(this);
		buttonRightTurn.setOnClickListener(this);
		buttonDown.setOnClickListener(this);
		buttonStop.setOnClickListener(this);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public void onClick(View arg0) {
		if(arg0 == buttonConnect)
		{
			imm.hideSoftInputFromWindow(editTextIPAddress.getWindowToken(), 0);
			
			try{
				if(socket!=null)
				{
					socket.close();
					socket = null;
				}
				
				server = editTextIPAddress.getText().toString();
				socket = new Socket(server, port);
				outs = socket.getOutputStream();

				rcvThread = new Thread(new rcvthread(logger, socket));
    		    rcvThread.start();
				logger.log("Connected");
			} catch (IOException e){
				logger.log("Fail to connect");
				e.printStackTrace();
			}
		}
		
		if(arg0 == buttonClose)
		{
			imm.hideSoftInputFromWindow(editTextIPAddress.getWindowToken(), 0);
			
			if(socket!=null)
			{
				exitFromRunLoop();
				try{
					socket.close();
					socket = null;
					logger.log("Closed!");
					rcvThread = null;
				} catch (IOException e){
					logger.log("Fail to close");
					e.printStackTrace();
				}
			}
		}
		
		if(arg0 == buttonUp || arg0 == buttonLeftTurn || 
		   arg0 == buttonRightTurn || arg0 == buttonDown || arg0 == buttonStop )
		{
			String sndOpkey = "CMD";
			
			if(arg0 == buttonUp)	    sndOpkey = "Up";
			if(arg0 == buttonLeftTurn)	sndOpkey = "LeftTurn";
			if(arg0 == buttonRightTurn)	sndOpkey = "RightTurn";
			if(arg0 == buttonDown)	    sndOpkey = "Down";
			if(arg0 == buttonStop)	    sndOpkey = "Stop";

			try{
				outs.write(sndOpkey.getBytes("UTF-8"));
				outs.flush();
			} catch (IOException e){
				logger.log("Fail to send");
				e.printStackTrace();
			}
		}
        if(arg0 ==null) {
            String sndOpkey = "free";
            try {
                outs.write(sndOpkey.getBytes("UTF-8"));
                outs.flush();
            }catch(IOException e){
                logger.log("Fail to send");
                e.printStackTrace();
            }


        }

	}
	
    void exitFromRunLoop(){
    	try {
    		String sndOpkey = "[close]";
    		outs.write(sndOpkey.getBytes("UTF-8"));
    		outs.flush();
    	} catch (IOException e) {
			logger.log("Fail to send");
			e.printStackTrace();
    	}
    }
}
