/*
 * Copyright 2011 Sergey Kolotsey.
 *
 * This file is part of AIRSerial library.
 *
 * AIRSerial is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIRSerial is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licenise
 * along with AIRSerial. If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================
 * 
 */

package ws.equator.proto{
	import flash.errors.IOError;
	import flash.net.ObjectEncoding;
	import flash.utils.Endian;
	import flash.utils.IDataOutput;
	import flash.utils.IDataInput;
	import flash.utils.ByteArray;
	import flash.events.ErrorEvent;
	import flash.system.Capabilities;
	import flash.desktop.NativeApplication;
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.events.NativeProcessExitEvent;
	import flash.events.ProgressEvent;
	import flash.desktop.NativeProcessStartupInfo;
	import flash.filesystem.File;
	import flash.desktop.NativeProcess;
	
	/**
	 * Class Serial works with serial ports in AIR applications
	 */
	public class Serial extends EventDispatcher implements IDataInput, IDataOutput{
				
		/**
		 * We use this function to override standard <code>toString()</code> method
		 * and return something recognizable
		 */
		override public function toString():String{
			return "Serial-"+_port;
		}
	
		/**
		 * Function is used to parse <code>io-serial</code> output when enumerating
		 * system serial ports. Function receives a string containing a list
		 * of serial ports and tranforms it to an array.
		 */
		static private function parseLines( string:String):Array{
			var p1:int, p2:int, p:int;
			var line:String;
			var ret:Array=new Array();
			
			do{
				p1=string.indexOf("\r");
				p2=string.indexOf("\n");
				if(p1!=-1 && p2!=-1){
					p=Math.min(p1, p2);
				}else if( p1 !=-1){
					p=p1;
				}else if( p2 !=-1){
					p=p2;
				}else{
					p=-1;
				}
				if( p==-1){
					if( string !=""){
						ret.push( string);
					}
				}else{
					line=string.substr(0, p);
					string=string.substr(p+1);
					if( line !=""){
						ret.push(line);
					}
				}
			}while( p !=-1);
			
			if(ret.length){
				ret.sort(Array.CASEINSENSITIVE);
			}
			return ret;
		}
		
		/**
		 * This function enumerates system serial ports. The function calls 
		 * io-serial executable to do the actual work, receives its output
		 * and parses it using function <code>parseLines()</code>. 
		 * After the lines are parsed and the list of serial ports is acquired,
		 * this function calls <code>callback</code> function and passes status
		 * and the list of serial ports as an array to it.
		 * 
		 * @param callback Callback function that should be called when
		 * the list of serial ports is acquired. This callback function
		 * must have the following declaration:
		 * <listing version="3.0">function callback( status:Boolean, list:Array);</listing>
		 * where <code>status</code> is true if function <code>enum()</code> 
		 * succeded and <code>list</code> is array of strings containing the 
		 * list of available serial ports.
		 * 
		 * @return Function execution status. Function returns <code>true</code> 
		 * if io-serial executable is executed and its output is pending.
		 * If the application does not support <code>NativeProcess</code>, then
		 * the function returns <code>false</code>.
		 * 
		 * @example The following example enumerates all serial ports in the 
		 * system
		 * <listing version="3.0">
		 * Serial.enum( function( success:Boolean, list:Array):void{
		 *    if( !success){
		 *        trace( "Could not enumerate serial ports");
		 *    }else{
		 *        if( list.length==0){
		 *            trace("There are no serial ports in the system);
		 *        }else{
		 *            for( var i:int=0; i&lt;list.length; i++){
		 *                trace( list[i]);
		 *            }
		 *        }
		 *    }
		 * });
		 * </listing>
		 * 
		 * @see flash.desktop.NativeProcess
		 */
		static public function enum( callback:Function):Boolean{
			if( !NativeProcess.isSupported){
				callback(false, []);
				return false;
				
			}else{
				BinaryExtractor.extract( function( success:String, serial:File):void{
					if( success){
						if( !NativeProcess.isSupported){
							 callback(false, []);
							 
						}else{
							var info:NativeProcessStartupInfo = new NativeProcessStartupInfo();
							var process:NativeProcess;
							var processData:String="";
							
							info.executable = serial;
							info.arguments=new <String>[ "enum"];
							process=new NativeProcess();
							process.addEventListener(ProgressEvent.STANDARD_OUTPUT_DATA, function(ev:ProgressEvent):void{
								processData+=(ev.target as NativeProcess).standardOutput.readUTFBytes((ev.target as NativeProcess).standardOutput.bytesAvailable);
							});
							process.addEventListener(NativeProcessExitEvent.EXIT, function( ev:NativeProcessExitEvent):void{
								if( ev.exitCode ==0){
									callback( true, parseLines( processData));
								}else{
									callback( false, []);
								}
							});
							
							try{
								process.start(info);
							}catch( e: Error){
								callback( false, []);
							}
						}
						
					}else{
						callback(false, []);
					}
				});
				return true;
			}
		}

 
		static private var encoding:String=Encoding.getNativeEncoding();
		protected var buffer:ByteArray;
		private var process:NativeProcess;
		private var _bytesWritten:uint;
		private var _bytesRead:uint;
		private var _speed:uint;
		private var _port:String;
		private var stderr:String;
		
		//Fields that need to implement IDataInput, IDataOutput
		private var _endian:String;
		private var _objectEncoding:uint;
		public var serialBytes:ByteArray;
		public function set port( arg:String):void{
			if( process !=null && process.running){
				throw new Error( "Port is already opened");
			}else{
				if( null==arg){
					throw new Error("Invalid port assigned");
				}else{
					_port=arg;
				}
			}	
		}
		
		public function get bytesWritten():uint{
			return _bytesWritten;
		}
		
		public function get bytesRead():uint{
			return _bytesRead;
		}
		
		public function get port():String{
			return _port;
		}
		
		public function set speed(arg:uint):void{
			if( process !=null && process.running){
				throw new Error( "Port is already opened");
			}else{
				if( arg==0){
					throw new Error( "Invalid speed assigned");
				}else{
					_speed=arg;
				}
			}
		}
		
		public function get speed():uint{
			return _speed;
		}
					
		private function processError( ev:ProgressEvent):void{
			var output:String, line:String;
			var i:int, j:int, k:int;
			
			if( encoding==null){
				output = process.standardError.readUTFBytes( process.standardError.bytesAvailable);
			}else{
				output = process.standardError.readMultiByte( process.standardError.bytesAvailable, encoding);
			}
			
			do{
				i=output.indexOf('\r');
				j=output.indexOf('\n');
				if(i==-1 && j==-1){
					k=-1;
				}else if( i==-1){
					k=j;
				}else if( j==-1){
					k=i;
				}else{
					k=Math.min( i, j);
				}
				if( k==-1){
					line=output;
				}else{
					line=output.substr(0, k);
					output=output.substr(k+1);
				}
				if( line !=""){
					if(stderr !="") stderr+="\n";
					stderr+=line;
				}
			}while( k!=-1);
		}
		
		protected function read( bytes:ByteArray):void{
			buffer.clear();
			buffer.writeBytes( bytes, 0, bytes.length);
			dispatchEvent(new ProgressEvent(ProgressEvent.PROGRESS));
		}
		
		protected function write( bytes:ByteArray, offset:uint=0, length:uint=0):void{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				var l:uint=length? length : bytes.length;
				process.standardInput.writeBytes( bytes, offset, length);
				_bytesWritten+=l;
			}
		}
		 
		private function processData(ev:ProgressEvent):void{
			var bytes:ByteArray=new ByteArray();
			process.standardOutput.readBytes( bytes);
			_bytesRead+=bytes.length;
			serialBytes = new ByteArray(bytes);
			read( bytes);
			
		}
				
		private function processExit(ev:NativeProcessExitEvent):void{
			if( isNaN(ev.exitCode) || ev.exitCode==0){
				trace("- Close port "+_port);
				dispatchEvent( new Event(Event.CLOSE));
				
			}else{
				//error occured in native process
				dispatchEvent( new ErrorEvent( ErrorEvent.ERROR, false, false, stderr));
			}
			process=null;
		}
		
		private function startProcess( serial:File):void{
			var info:NativeProcessStartupInfo = new NativeProcessStartupInfo();
			
			info.executable = serial;
			info.workingDirectory=File.applicationStorageDirectory;
			info.arguments=new <String>[ "speed="+_speed, _port];
			process=new NativeProcess();
			process.addEventListener(ProgressEvent.STANDARD_OUTPUT_DATA, processData);
			process.addEventListener(ProgressEvent.STANDARD_ERROR_DATA, processError);
			process.addEventListener(NativeProcessExitEvent.EXIT, processExit);
			process.standardInput.endian=_endian;
			process.standardInput.objectEncoding=_objectEncoding;
			_bytesWritten=0;
			_bytesRead=0;
			stderr="";
			try{
				process.start(info);
			}catch( e: Error){
				dispatchEvent( new ErrorEvent( ErrorEvent.ERROR, false, false, "Start process error: "+e.message));
				process=null;
			}
			trace("+ Open port "+_port);
			if( null !=process){
				dispatchEvent( new Event(Event.OPEN));
			}
		}
		
		public function close():void{
			if( null !=process){
				if(process.running){
					process.exit();
				}
			}		
		}
				
		public function open():void{
			if( process !=null && process.running){
				return;
			}
			if( !NativeProcess.isSupported){
				dispatchEvent( new ErrorEvent( ErrorEvent.ERROR, false, false, "NativeProcess is NOT supported"));
				
			}else{
				
				BinaryExtractor.extract( function( success:Boolean, serial:File):void{
					if( success){
						
						
						startProcess( serial);
					}else{
						dispatchEvent( new ErrorEvent( ErrorEvent.ERROR, false, false, BinaryExtractor.error));
					}
				});
			}
		}
		
		private function nativeApplicationExit( event: Event): void{
			close();
		}
		
		public function Serial(){
			NativeApplication.nativeApplication.addEventListener( Event.EXITING, nativeApplicationExit);
			process=null;
			_speed=9600;
			if (Capabilities.os.toLowerCase().indexOf("win") !=-1){
				_port="COM1";
			}else{
				_port="/dev/ttyS0";
			}
			buffer=new ByteArray();
			_endian=Endian.BIG_ENDIAN;
			_objectEncoding=ObjectEncoding.DEFAULT;
			buffer.endian=_endian;
			buffer.objectEncoding=_objectEncoding;
		}


		//The following functions implement IDataInput, IDataOutput interfaces
		
		public function get bytesAvailable(): uint{
			return buffer.bytesAvailable;
		}

		public function get endian(): String{
			return _endian;
		}

		public function set endian( type: String): void{
			switch( type){
				case Endian.BIG_ENDIAN:
				case Endian.LITTLE_ENDIAN:
					_endian=type;
					break;
				default:
					_endian=Endian.BIG_ENDIAN;
					break;
			}
			if( process !=null) process.standardInput.endian=_endian;
			buffer.endian=_endian;
		}

		public function get objectEncoding(): uint{
			return _objectEncoding;
		}

		public function set objectEncoding( version: uint): void{
			switch( version){
				case ObjectEncoding.AMF0:
				case ObjectEncoding.AMF3:
					_objectEncoding=version;
					break;
				default:
					_objectEncoding=ObjectEncoding.DEFAULT;
					break;
			}
			if( process !=null) process.standardInput.objectEncoding=_objectEncoding;
			buffer.objectEncoding=_objectEncoding;
		}

		/**
		 * Reads a Boolean value from the byte stream. 
		 * A single byte is read, and <code>true</code> is returned if the byte is nonzero, 
		 * <code>false</code> otherwise.
		 * @return Returns <code>true</code> if the byte is nonzero, 
		 * <code>false</code> otherwise.
		 * @throws EOFError There is not sufficient data available to read.
		 */
		public function readBoolean(): Boolean{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readBoolean(); 
			}
		}

		/**
		 * @copy flash.utils.ByteArray#readByte()
		 */
		public function readByte(): int{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readByte();
			}
		}

		public function readBytes( bytes: ByteArray, offset: uint = 0, length: uint = 0): void{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				buffer.readBytes( bytes, offset, length);
			}
		}

		public function readDouble(): Number{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readDouble();
			}
		}

		public function readFloat(): Number{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readFloat();
			}
		}

		public function readInt(): int{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readInt();
			}
		}

		public function readMultiByte( length: uint, charSet: String): String{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readMultiByte( length, charSet);
			}
		}

		public function readObject(): *{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readObject();
			}
		}

		public function readShort(): int{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readShort();
			}
		}

		public function readUTF(): String{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readUTF();
			}
		}

		public function readUTFBytes( length: uint): String{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readUTFBytes( length);
			}
		}

		public function readUnsignedByte(): uint{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readUnsignedByte();
			}
		}

		public function readUnsignedInt(): uint{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readUnsignedInt();
			}
		}

		public function readUnsignedShort(): uint{
			if( process==null){
				throw new IOError( "Port is not opened");
			}else{
				return buffer.readUnsignedShort();
			}
		}

		public function writeBoolean( value: Boolean): void{
			var b:ByteArray=new ByteArray();
			b.writeBoolean( value);
			write( b); 
		}

		public function writeByte( value: int): void{
			var b:ByteArray=new ByteArray();
			b.writeByte( value);
			write( b);
		}

		public function writeBytes( bytes: ByteArray, offset: uint = 0, length: uint = 0): void{
			write( bytes, offset, length);
		}

		public function writeDouble( value: Number): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeDouble( value);
			write( b);
		}

		public function writeFloat( value: Number): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeFloat( value);
			write( b);
		}

		public function writeInt( value: int): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeInt( value);
			write( b);
		}

		public function writeMultiByte( value: String, charSet: String): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeMultiByte( value, charSet);
			write( b);
		}

		public function writeObject( object: *): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.objectEncoding=_objectEncoding;
			b.writeObject( object);
			write( b);
		}

		public function writeShort( value: int): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeShort( value);
			write( b);
		}

		public function writeUTF( value: String): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeUTF( value);
			write( b);
		}

		public function writeUTFBytes( value: String): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeUTFBytes( value);
			write( b);
		}

		public function writeUnsignedInt( value: uint): void{
			var b:ByteArray=new ByteArray();
			b.endian=_endian;
			b.writeUnsignedInt( value);
			write( b);
		}
	}
}
