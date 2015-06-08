import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;s
import java.net.SocketAddress;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.pi4j.io.gpio.GpioController;
import com.pi4j.io.gpio.GpioFactory;
import com.pi4j.io.gpio.GpioPinDigitalOutput;
import com.pi4j.io.gpio.PinState;
import com.pi4j.io.gpio.RaspiPin;

public class RcvThread implements Runnable{
    private static final int sizeBuf = 50;//���۸� ���� ������ ����
    private Socket clientSock;//Ŭ���̾�Ʈ ��Ĺ��ü ����
    private Logger logger;///�α׸� ����� ���� ��ü
    private SocketAddress clientAddress;// ��Ĺ ��巹���� ������ ��ü

    final GpioController gpio = GpioFactory.getInstance();//GPIO����
	///RPCī�� ���� ����̺긦 �����ϱ� ���� PIN ����
    final GpioPinDigitalOutput PinOne = gpio.provisionDigitalOutputPin(RaspiPin.GPIO_01, "pin1", PinState.LOW);
    final GpioPinDigitalOutput PinTwo = gpio.provisionDigitalOutputPin(RaspiPin.GPIO_02, "pin2", PinState.LOW);
    final GpioPinDigitalOutput PinThree = gpio.provisionDigitalOutputPin(RaspiPin.GPIO_03, "pin3", PinState.LOW);
    final GpioPinDigitalOutput PinFour = gpio.provisionDigitalOutputPin(RaspiPin.GPIO_04, "pin4", PinState.LOW);
    

    public RcvThread(Socket clntSock, SocketAddress clientAddress, Logger logger) {
        this.clientSock = clntSock;
        this.logger = logger;
        this.clientAddress = clientAddress;
    }

    public void run(){
        try {
            InputStream ins   = clientSock.getInputStream();///���۵� �Է°��� ������ ��ü
            OutputStream outs = clientSock.getOutputStream();///�Էµ� ���� �ٽ� ������ ���� ��ü

            int rcvBufSize;//���� ������ 
            byte[] rcvBuf = new byte[sizeBuf];
            while ((rcvBufSize = ins.read(rcvBuf)) != -1) {//���ۻ���� 0�̻��̸� �ݺ����� �����Ѵ�. �������� �� ��ư�� ������ ���� �����̰� �ȴ�.

                String rcvData = new String(rcvBuf, 0, rcvBufSize, "UTF-8");

                if (rcvData.compareTo("Up") == 0) {
                 	PinOne.low();
               	PinTwo.high();
                }

                if (rcvData.compareTo("LeftTurn") == 0) {
              	PinThree.high();
		PinFour.low();

                    	try{Thread.sleep(200);}
                    	catch(InterruptedException e) {}

                   	PinThree.low();
		PinFour.low();
                }

                if (rcvData.compareTo("RightTurn") == 0) {
               	PinThree.low();
		PinFour.high();

	              try{Thread.sleep(200);}
                    	catch(InterruptedException e) {}

	              pinlpwm.low();
                    	pinrpwm.low();
                }
                if (rcvData.compareTo("Down") == 0) {
                	PinOne.high();
                	PinTwo.low();
                }

                if (rcvData.compareTo("Stop") == 0) {
                   	PinOne.low();
                   	PinTwo.low();
                     
                }
                logger.info("Received data : " + rcvData + " (" + clientAddress + ")");
                outs.write(rcvBuf, 0, rcvBufSize);
            }
            logger.info(clientSock.getRemoteSocketAddress() + " Closed");
        } catch (IOException ex) {
            logger.log(Level.WARNING, "Exception in RcvThread", ex);
        } finally {
            try {
                clientSock.close();
                System.out.println("Disconnected! Client IP : " + clientAddress);
            } catch (IOException e) {}
        }
    }
}

