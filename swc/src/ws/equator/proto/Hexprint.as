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
 * This class provides support for converting binary data into human-readable
 * format. Any binary data passed to the static function Hexprint.hexprint() is 
 * convertedinto a string that represents the data in hexademical numbers. 
 * 
 */
 
package ws.equator.proto{
	import flash.utils.ByteArray;

	public class Hexprint{
		public static function hexprint( data:ByteArray, offset:uint=0, length:uint=0):String{
			var symbol:uint;
			var ret:String="";
			var saved_offset:uint=data.position;
			
			if( offset>data.length) offset=data.length;
			data.position=offset;
			if( length==0 || length > data.bytesAvailable) length=data.bytesAvailable;
			while( length){
				length--;
				symbol=data.readUnsignedByte();
				if(ret.length) ret +="-";
				ret+=(symbol>0x0f? "" : "0") + symbol.toString(16);
			}
			data.position=saved_offset;
			return ret;
		}
	}
}
