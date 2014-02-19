package  {
	
	import flash.desktop.NativeProcess;
	import flash.desktop.NativeProcessStartupInfo;
	import flash.display.MovieClip;
	import flash.events.ErrorEvent;
	import flash.events.Event;
	import flash.events.KeyboardEvent;
	import flash.events.MouseEvent;
	import flash.events.NativeProcessExitEvent;
	import flash.events.TimerEvent;
	import flash.filesystem.File;
	import flash.net.FileReference;
	import flash.system.Capabilities;
	import flash.text.TextField;
	import flash.ui.Keyboard;
	import flash.ui.Mouse;
	import flash.utils.ByteArray;
	import flash.utils.Timer;
	import flash.utils.getQualifiedClassName;
	import flash.utils.setInterval;
	import flash.utils.setTimeout;
	
	import avmplus.getQualifiedClassName;
	
	import fl.controls.Button;
	import fl.controls.ComboBox;
	import fl.controls.Label;
	import fl.controls.TextArea;
	import fl.controls.TextInput;
	import fl.data.DataProvider;
	
	import ws.equator.proto.Serial;

	public class AIRSerialGUI extends MovieClip {
		
		public var infoTxt:TextArea;
		public var gcodeTxt:TextArea;
		public var sendTxt:TextInput;
		public var intervalTxt:TextInput;
		public var fileTxt:Label;
		public var listCb:ComboBox;
		public var connectBt:Button;
		public var sendBt:Button;
		public var loadBt:Button;
		public var loadMusicBt:Button;
		public var saveBt:Button;
		public var startBt:Button;
		
		private var _serial:Serial;
		private var _isConnected:Boolean = false;
		private var _file:File = new File();
		private var _timer:Timer = new Timer(1000);
		private var _listGCode:Array = [];
		private var _index:uint = 0;
		private var _isSave:Boolean = false;
		private var _isMusic:Boolean = false;
		public function AIRSerialGUI() {
			// constructor code
			Serial.enum(serial_list);
			connectBt.addEventListener(MouseEvent.CLICK,onConnectClick);
			sendBt.addEventListener(MouseEvent.CLICK,onSendClick);
			loadBt.addEventListener(MouseEvent.CLICK,onLoadClick);
			loadMusicBt.addEventListener(MouseEvent.CLICK,onLoadMusicClick);
			saveBt.addEventListener(MouseEvent.CLICK,onSaveClick);
			_file.addEventListener(Event.SELECT,onFileSelected);
			_file.addEventListener(Event.COMPLETE,onFileComplete);
			_timer.addEventListener(TimerEvent.TIMER,onTimer);
			_timer.start();
			startBt.addEventListener(MouseEvent.CLICK,onStartClick);
			stage.addEventListener(KeyboardEvent.KEY_UP,onKeyUpHandle);
			trace(NativeProcess.isSupported);
//			bt.addEventListener(MouseEvent.CLICK,onClick);
//			this.addEventListener(Event.ENTER_FRAME,onLoop);
		}
		private function onConnectClick(evt:MouseEvent):void{
			if(connectBt.label=="Connect"){
				if(listCb.selectedItem.label.length>10||Capabilities.os.toLowerCase().indexOf("win")>-1){
					connectBt.label = "Connecting";
					_serial = new Serial();
					_serial.addEventListener(Event.OPEN,onCom);
					_serial.addEventListener(ErrorEvent.ERROR,onError);
					_serial.port = listCb.selectedItem.label;
					_serial.speed = 9600;
					_serial.open();
				}
			}else{
				_serial.close();
				connectBt.label = "Connect";
				infoTxt.text = "disconnected";
			}
		}
		private function onLoadClick(evt:MouseEvent):void{
			_file.browse();
			_isSave = false;
			_isMusic = false;
		}
		private function onLoadMusicClick(evt:MouseEvent):void{
			_file.browse();
			_isSave = false;
			_isMusic = true;
		}
		private function onSaveClick(evt:MouseEvent):void{
			_file.save(gcodeTxt.text,"gcode.txt");
			_isSave = true;
		}
		private function onFileSelected(evt:Event):void{
			if(_isSave){
				
			}else{
				_file.load();
				fileTxt.text = _file.name;
			}
		}
		private function onFileComplete(evt:Event):void{
			if(_isSave){
				
			}else{
				var bytes:ByteArray = _file.data;
				bytes.position = 0;
				var string:String = bytes.readUTFBytes(bytes.length);
				if(_isMusic){
					parseMusicCode(string);
				}else{
					gcodeTxt.text = string;
					_listGCode = string.split("\n");
					if(_listGCode.length==1){
						_listGCode = string.split("\r");
					}
					_index = 0;
				}
			}
		}
		private function onStartClick(evt:MouseEvent):void{
			if(startBt.label == "Start"){
				startBt.label = "Stop";
			}else{
				startBt.label = "Start";
			}
		}
		private function onTimer(evt:TimerEvent):void{
			if(_isConnected){
				if(startBt.label == "Stop"){
					infoTxt.text = (_index+1)+":"+_listGCode[_index];
					if(_listGCode[_index].indexOf("delay")>-1){
						_timer.delay = Number(_listGCode[_index].split(" ")[1]);
						_isMusic = true;
					}else{
						_serial.writeUTFBytes(""+_listGCode[_index]+"/n");
					}
					_index++;
					if(_index>=_listGCode.length){
						startBt.label = "Start";
						_index = 0;
					}
				}
			}
			if(!_isMusic){
				_timer.delay = Math.max(100,Number(intervalTxt.text));
			}
		}
		private function onSendClick(evt:MouseEvent):void{
			if(_isConnected){
				_serial.writeUTFBytes(""+sendTxt.text+"/n");
			}
		}
		private function serial_list( success:Boolean, systemPorts:Array):void{
			if(success){
				var dataProvider:DataProvider = new DataProvider();
				for(var i in systemPorts){
					if(systemPorts[i].length>10||Capabilities.os.toLowerCase().indexOf("win")>-1){
						dataProvider.addItem({data:i,label:systemPorts[i]});
					}
				}
				listCb.dataProvider = dataProvider;
			}else{
				 
				infoTxt.text = "init Serial fail";
			}
		}
		private function onCom(evt:Event):void{
			_isConnected = true;
			connectBt.label = "Disconnect";
			infoTxt.text = "connected";
		}
		private function onError(evt:ErrorEvent):void{
			_isConnected = false;
			connectBt.label = "Connect";
			infoTxt.text = "disconnected";
		}
		private function sendMusic(index:uint):void{
			
				if(index==0){
					gcodeTxt.appendText("\n");
					return;
				}
				gcodeTxt.appendText("G1 X-"+(0.22*(16-index)+0.1)+"\n");
				gcodeTxt.appendText("G1 Z1\n");
				setTimeout(function(){ _serial.writeUTFBytes("G1 X-"+(0.22*(16-index)+0.1)+"/n");},10);
				setTimeout(function(){ _serial.writeUTFBytes("G1 Z0/n");},210);
		}
		private function onKeyUpHandle(evt:KeyboardEvent):void{
			if(evt.keyCode == Keyboard.ESCAPE){
				saveBt.setFocus();
			}
			if(!_isConnected){
				return;
			}
			var obj:* = sendTxt.getFocus();
			if(getQualifiedClassName(obj)!="flash.text::TextField"){
				switch(String.fromCharCode(evt.charCode)){
					
					case "0":
						sendMusic(0);
						break;
					case "1":
						sendMusic(1);
						break;
					case "2":
						sendMusic(2);
						break;
					case "3":
						sendMusic(3);
						break;
					case "4":
						sendMusic(4);
						break;
					case "5":
						sendMusic(5);
						break;
					case "6":
						sendMusic(6);
						break;
					case "7":
						sendMusic(7);
						break;
					case "q":
						sendMusic(8);
						break;
					case "w":
						sendMusic(9);
						break;
					case "e":
						sendMusic(10);
						break;
					case "r":
						sendMusic(11);
						break;
					case "t":
						sendMusic(12);
						break;
					case "y":
						sendMusic(13);
						break;
					case "u":
						sendMusic(14);
						break;
					case "a":
						sendMusic(15);
						break;
				}
			}
		}
		private function parseMusicCode(str:String):void{
			var arr:Array = str.split(" ");
			var params:Array;
			var prevIndex:uint = 15;
			var index:uint = 0;
			addMsg("delay 1000");
			var delay:Number = Math.max(100,Number(intervalTxt.text));
			for(var i:uint=0;i<arr.length;i++){
				params = arr[i].split("|");
				params[0] = Number(params[0]);
				params[1] = Number(params[1]);
				if(params[0]<0){
					index = -params[0];
					addMsg("G1 X-"+(0.22*(16-index)+0.1));
					addMsg("delay "+(Math.abs(delay*(prevIndex-index)/4)));
					addMsg("G1 Z1");
					if(params[1]==0){
						addMsg("delay "+delay);
					}else{
						if(params[1]>0){
							addMsg("delay "+(delay*(1+1/params[1])));
						}else{
							addMsg("delay "+(-delay*(1/params[1])));
						}
					}
				}else{
					index = params[0]+7;
					addMsg("G1 X-"+(0.22*(16-index)+0.1));
					addMsg("delay "+(Math.abs(delay*(prevIndex-index)/4)));
					addMsg("G1 Z1");
					if(params[1]==0){
						addMsg("delay "+delay);
					}else{
						if(params[1]>0){
							addMsg("delay "+(delay*(1+1/params[1])));
						}else{
							addMsg("delay "+(-delay*(1/params[1])));
						}
					}
				}
				prevIndex = index;
			}
		}
		private function addMsg(msg:String):void{
			gcodeTxt.appendText(msg+"\n");
		}
	}
	
}
