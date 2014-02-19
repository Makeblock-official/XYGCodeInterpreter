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
 * The static function getNativeEncoding() from this class returns native
 * encoding for Windows console.
 */
 
package ws.equator.proto{
	import flash.system.Capabilities;

	internal class Encoding{
		static public function getNativeEncoding():String{
			if( Capabilities.os.toLowerCase().indexOf("win") !=-1){
				switch( Capabilities.language){
					case "cs"://Czech
					case "pl"://Polish
					case "hu"://Hungarian
						return "windows-1250";
					case "ru"://Russian
						return "windows-1251";
					case "tr":
						return "windows-1254";
					case "it"://Italian
					case "es"://Spanish
					case "pt"://Portuguese
					case "fr"://French
					case "de"://German
					case "nl"://Dutch
					case "da"://Danish
					case "sv"://Swedish
					case "no"://Norvegian
					case "fi"://Finnish
						return "windows-1252";
					case "ko"://Korean
						return "euc-kr";
					case "ja"://Japann
						return "shift_jis";
					case "zh-CN"://Simplified Chinese
						return "EUC-CN";
					case "zh-TW"://Traditional Chinese
						return "big5";
					default:
						return null;
				}
			}else{
				return null;
			}
		}
	}
}
