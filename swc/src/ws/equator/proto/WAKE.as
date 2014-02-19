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
 * 
 */
 
package ws.equator.proto{
	import flash.events.Event;
	import flash.events.ProgressEvent;
	import flash.events.IOErrorEvent;
	import flash.utils.ByteArray;

	public class WAKE extends Serial{
		static private const FEND:uint=0xC0;
		static private const FESC:uint=0xDB;
		static private const TFEND:uint=0xDC;
		static private const TFESC:uint=0xDD;
		
		static private const WAKE_PACKET_WAIT:uint=		0;
		static private const WAKE_ADDRESS:uint=			1;
		static private const WAKE_COMMAND:uint=			2;
		static private const WAKE_LENGTH:uint=			3;
		static private const WAKE_DATA:uint=			4;
		static private const WAKE_CRC:uint=				5;
		static private const WAKE_PACKET_READY:uint=	6;
		
		
		private var use_crc:Boolean=true;
		private var crc:CRC;
		private var addr:uint=0;
		private var packet_address:uint=0;
		private var packet_command:uint=0;
		private var packet_length:uint=0;
		private var state:uint=WAKE_PACKET_WAIT;
		private var escape:Boolean=false;
		private var _bytesWritten:uint;
		private var _bytesRead:uint;
		
		
		

		
		protected function read_done():void{
			dispatchEvent( new ProgressEvent( ProgressEvent.PROGRESS));
		}
		
		public function get packetAddress():uint{
			return packet_address;
		}
		
		public function get packetCommand():uint{
			return packet_command;
		}
		
		override protected function read(bytes:ByteArray):void{
			var symbol:uint;
			var error:String;
			
			while( bytes.bytesAvailable){
				symbol=bytes.readUnsignedByte();
				error=null;
				
				if( symbol == FEND){ 
					state = WAKE_ADDRESS;
					escape=false;
					packet_address=0;
					packet_command=0;
					packet_length=0;
					buffer.clear();
					if( use_crc){
						crc=new CRC( CRC.CRC_METHOD_DALLAS);
						crc.update( FEND);
					}
					
				}else if( state !=WAKE_PACKET_WAIT){
					if(symbol==FESC){
						escape=true;
						
					}else{
						if( escape){
							escape=false;
							if( symbol==TFESC){
								symbol=FESC;
							}else if( symbol==TFEND){
								symbol=FEND;
							}else{
								error="Invalid escape character received";
								state=WAKE_PACKET_WAIT;
							}
						}
						
						switch( state) {
							case WAKE_ADDRESS:
								if( symbol & 0x80){
									//address is present in command
									if(symbol & 0x7f){ 
										if( addr==(symbol & 0x7f)){
											packet_address=addr;
											if( use_crc) crc.update( addr);
											state = WAKE_COMMAND;
										}else{
											//Skip this packet, because it is
											//addressed to another device
											state=WAKE_PACKET_WAIT;
										}
									}else{
										//Broadcast command with address=0
										if( use_crc) crc.update( 0x00);
										state = WAKE_COMMAND;
									}
									break;
								}//else if ( ! (symbol & 0x80))
								 //No address field found in packet.
								 //The packet is sent to broadcast address
								 //Fall down and process this field as command field
							
							case WAKE_COMMAND:
								if( symbol & 0x80){
									//Invalid field: in the command field MSB should be 0
									error="Invalid command field received";
									state=WAKE_PACKET_WAIT;
								}else{
									packet_command=symbol;
									if( use_crc) crc.update( symbol);
									state=WAKE_LENGTH;
								}
								break;
								
							case WAKE_LENGTH:
								packet_length=symbol;
								if( use_crc) crc.update( symbol);
								if( packet_length){
									state=WAKE_DATA;
								}else if( use_crc){
									state=WAKE_CRC;
								}else{
									state=WAKE_PACKET_READY;
								}
								break;
								
							case WAKE_DATA:
								buffer.writeByte( symbol);
								if(use_crc) crc.update( symbol);
								packet_length--;
								if( 0==packet_length){
									if( use_crc){
										state=WAKE_CRC;
									}else{
										state=WAKE_PACKET_READY;
									}
								}
								break;
					
							case WAKE_CRC:
								var crc_value:uint=crc.finalize();
								if( symbol==crc_value){
									state=WAKE_PACKET_READY;
								}else{
									error="CRC field in packet does not match real CRC. Packet dropped";
									state=WAKE_PACKET_WAIT;
								}
								break;

							default:
								state = WAKE_PACKET_WAIT;
								break;
						}
					}
				}
				
				if(error !=null){
					dispatchEvent( new IOErrorEvent( IOErrorEvent.IO_ERROR, false, false, error));
					state = WAKE_PACKET_WAIT; 
					
				}else if( state==WAKE_PACKET_READY){
					_bytesRead+=buffer.length; 
					buffer.position=0;
					read_done();
					state=WAKE_PACKET_WAIT;
				} 
			}
		}
		
		public function writeTo( address:int, command:uint, bytes:ByteArray, offset:uint=0, length:uint=0):void{
			if( address>0x7f){
				throw new ArgumentError( "Address must be in the range 0..127");
				
			}else if(command>0x7f){
				throw new ArgumentError( "Command must be in the range 0..127");
				
			}else{
				var symbol:uint; 
				var output:ByteArray=new ByteArray();
				var saved_position:uint=bytes.position;
				
				if( offset) offset=Math.min( offset, bytes.length);
				bytes.position=offset;
				if( length) length=Math.min( length, bytes.bytesAvailable);
				
				output.endian=super.endian;
				output.objectEncoding=super.objectEncoding;
				//Start Sequence byte
				output.writeByte( FEND);
				//Address byte may omitted if it equals 0
				address=address & 0x7f;
				if( address){
					if( (address | 0x80)==FEND){
						output.writeByte( FESC);
						output.writeByte( TFEND);
					}else if( (address | 0x80)==FESC){
						output.writeByte( FESC);
						output.writeByte( TFESC);
					}else{
						output.writeByte( address | 0x80);
					}
				}
				//Command byte may be 0x00 which is NOOP
				command = command & 0x7f;
				if( command==FEND){
					output.writeByte( FESC);
					output.writeByte( TFEND);
				}else if( command==FESC){
					output.writeByte( FESC);
					output.writeByte( TFESC);
				}else{
					output.writeByte( command);
				}
				//Length of the message
				if( length==FEND){
					output.writeByte( FESC);
					output.writeByte( TFEND);
				}else if( length==FESC){
					output.writeByte( FESC);
					output.writeByte( TFESC);
				}else{
					output.writeByte( length);
				}
				while( bytes.bytesAvailable){
					symbol=bytes.readUnsignedByte();
					if( symbol==FEND){
						output.writeByte( FESC);
						output.writeByte( TFEND);
					}else if( symbol==FESC){
						output.writeByte( FESC);
						output.writeByte( TFESC);	
					}else{
						output.writeByte( symbol);
					}
				}
				//CRC
				if(use_crc){
					var crc:CRC=new CRC( CRC.CRC_METHOD_DALLAS);
					var crc_value:uint;
					
					crc.update( FEND);
					if( address){
						crc.update( address);
					}
					crc.update( command);
					crc.update(length);
					bytes.position=offset;
					while( bytes.bytesAvailable){
						symbol=bytes.readUnsignedByte();
						crc.update( symbol);
					}
					crc_value=crc.finalize();
					
					if( crc_value==FEND){
						output.writeByte( FESC);
						output.writeByte( TFEND);
					}else if( crc_value==FESC){
						output.writeByte( FESC);
						output.writeByte( TFESC);	
					}else{
						output.writeByte( crc_value);
					}
				}
				
				super.write( output);
				_bytesWritten+=length; 
				
				bytes.position=saved_position;
			}
		}
		
		override protected function write( bytes:ByteArray, offset:uint=0, length:uint=0):void{
			//Send bradcast NOOP command with supplied data, so any device can receive it.
			writeTo( 0, 0, bytes, offset, length);
		}
		
		public function get encodedBytesWritten():uint{
			return _bytesWritten;
		}
		
		public function get encodedBytesRead():uint{
			return _bytesRead;
		}
		
		public function get crcEnabled():Boolean{
			return use_crc;
		}
		
		public function set crcEnabled(param:Boolean):void{
			use_crc=param;
		}
		
		public function get address():uint{
			return addr & 0x7f;
		}
		
		public function set address(param:uint):void{
			if( param>0x7f){
				throw new ArgumentError( "Address must be in the range 0..127");
			}else{
				addr=(param | 0x80) & 0xff;
			}
		}
		
		private function portOpen( ev:Event):void{
			super.removeEventListener( Event.OPEN, portOpen);
			_bytesRead=_bytesWritten=0;
		}
		
		public function WAKE():void{
			super();
			super.addEventListener( Event.OPEN, portOpen);
		}
	}
}
