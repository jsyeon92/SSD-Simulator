import android.os.Handler;
import android.widget.TextView;

class ps implements Runnable{
	TextView t;
	String s;
	public ps(TextView t, String s){
		this.t = t;
		this.s = s;
	}
	public void run(){
		t.setText(s);
	}
}

public class logger {
	Handler h;
	TextView t;
	public logger(TextView t){
		this.t = t;
		h = new Handler();
	}
	public void log(String s){
		h.post(new ps(t, s));
	}
}

