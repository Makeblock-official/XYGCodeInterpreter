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
 * This class embeds native executable files for all supported platforms and
 * extracts a proper executable when needed.
 */

package ws.equator.proto{
	import flash.events.Event;
	import flash.events.NativeProcessExitEvent;
	import flash.desktop.NativeProcessStartupInfo;
	import flash.filesystem.FileMode;
	import flash.system.Capabilities;
	import flash.utils.ByteArray;
	import flash.filesystem.FileStream;
	import flash.filesystem.File;
	import flash.desktop.NativeProcess;
	import flash.events.EventDispatcher;

	internal class BinaryExtractor extends EventDispatcher{
		[Embed(source="io-serial", mimeType="application/octet-stream")]
		static private const IOSerialLinux:Class;
		[Embed(source="io-serial-mac", mimeType="application/octet-stream")]
		static private const IOSerialMac:Class;
		[Embed(source="io-serial.exe", mimeType="application/octet-stream")]
		static private const IOSerialWindows:Class;
		static private const IOSerialBasename:String="io-serial";
		
		static private var binaryExtractor:BinaryExtractor=null;
		static private var binary:File=null;
		static private var success:Boolean;
		static public var error:String=null;
		
		
		static private function findChmod():File{
			var file:File=new File();
			var path:Array=["/bin", "/sbin", "/usr/bin", "/usr/sbin", "/usr/local/bin", "/usr/local/sbin"];
			var i:int;
			for( i=0; i<path.length; i++){
				file.nativePath=path[i]+"/chmod";
				if( file.exists) return file;
			}
			return null;
		}
		
		private function extractBinary( basename:String, binaryWindows:Class, binaryLinux:Class, binaryMacos:Class):void{
			if( !NativeProcess.isSupported){
				error = "NativeProcess is not supported";
				success=false;
				dispatchEvent( new Event( Event.COMPLETE));
				
			}else{
				var file:File=null;
				var stream:FileStream;
				var binaryData:ByteArray;
				
				if (Capabilities.os.toLowerCase().indexOf("win") !=-1){
					if( null !=binaryWindows){
	    				file = File.applicationStorageDirectory.resolvePath( basename+".exe");
	    				binaryData=new binaryWindows();
					}
				}else if (Capabilities.os.toLowerCase().indexOf("lin") !=-1){
					if( null !=binaryLinux){
						file = File.applicationStorageDirectory.resolvePath( basename);
						binaryData=new binaryLinux();
					}
				}else{
					if( null !=binaryMacos){
						file = File.applicationStorageDirectory.resolvePath( basename);
						binaryData=new binaryMacos();
					}
				}
				if( null==file){
					error = "OS is not supported";
					success=false;
					
					dispatchEvent( new Event( Event.COMPLETE));
				}else{
					
					stream=new FileStream();
					try{
						stream.open( file, FileMode.WRITE);
						stream.writeBytes( binaryData, 0, binaryData.bytesAvailable);
						stream.close();
					}catch(e:Error){
						error = "Could not extract binary";
						success=false;
						dispatchEvent( new Event( Event.COMPLETE));
					}
					
					if (Capabilities.os.toLowerCase().indexOf("lin") ==-1&&Capabilities.os.toLowerCase().indexOf("mac") ==-1){
						success=true;
						binary=file;
						dispatchEvent( new Event( Event.COMPLETE));
						
					}else{
						//chmod extracted file
						var chmod:File=findChmod();
						if( null==chmod){
							error = "Could not find chmod in the system";
							success=false;
							dispatchEvent( new Event( Event.COMPLETE));
							
						}else{
							var info:NativeProcessStartupInfo = new NativeProcessStartupInfo();
							var process:NativeProcess;
							
							info.executable = chmod;
							info.workingDirectory=File.applicationStorageDirectory;
							info.arguments=new <String>[ "+x", file.nativePath];
							process=new NativeProcess();
							process.addEventListener(NativeProcessExitEvent.EXIT, function( ev:NativeProcessExitEvent):void{
								if( ev.exitCode ==0){
									success=true;
									binary=file;
								}else{
									success=false;
									error="Chmod returned error: "+ev.exitCode;
								}
								dispatchEvent( new Event( Event.COMPLETE));
							});
							
							try{
								process.start(info);
							}catch( e: Error){
								error = "Could not call chmod: "+e.message;
								success=false;
								dispatchEvent( new Event( Event.COMPLETE));
							}
						}
					}
				}
			}
		}
	
		static public function extract( callback:Function):void{
			if( null==binary){
				if( null==binaryExtractor){
					binaryExtractor=new BinaryExtractor();
					binaryExtractor.addEventListener( Event.COMPLETE, function( ev:Event):void{
						callback( success, binary);
						binaryExtractor=null;
					});
					binaryExtractor.extractBinary( IOSerialBasename, IOSerialWindows, IOSerialLinux, IOSerialMac);
				}else{
					binaryExtractor.addEventListener( Event.COMPLETE, function( ev:Event):void{
						callback( success,  binary);
					});
				}
				
			}else{
				callback( success, binary);
			}
		}
	}
}
