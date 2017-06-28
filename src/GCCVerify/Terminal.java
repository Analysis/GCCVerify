/**
 * GCCVerifyTerminal.java is the actual console program that the user runs
 * to update/download manifests and firmware binaries, and run the verification.
 *
 * Contains the single main() function
 *
 * Copyright (C) 2017 Aram Akhavan <kaysond@hotmail.com>
 * https://github.com/kaysond/GCCVerify
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
package GCCVerify;
import GCCVerify.*;
import java.util.Scanner;
import jssc.*;

public class Terminal {

	public static void main(String[] args) {

		System.out.printf("%n-------------------%n");
		System.out.println("GCCVerify by Kayson");
		System.out.printf("-------------------%n%n");

		boolean offline = false;
		for (String arg : args) {
			if ( arg.equals("-d") ) {
				System.out.printf("Debug mode enabled.%n%n");
				GCCVerifier.debug = true;
				Manifest.debug = true;
			}
			if ( arg.equals("-o") ) {
				offline = true;
				System.out.printf("Offline mode enabled.%n%n");
			}
		}
			
		//Startup: Load manifests, check for updates
		boolean loadedRemote = false;
		if ( !GCCVerifier.loadLocalManifest() ) {
			if ( offline ) {
				System.out.println("ERROR: Could not load any manifest.");
				System.exit(1);
				
			}
			else {
				if ( !GCCVerifier.loadRemoteManifest() ) {
					System.out.println("ERROR: Could not load any manifest.");
					System.exit(1);
				}
				else {
					GCCVerifier.saveRemoteManifestToLocal();
					GCCVerifier.useLocalManifest();
					loadedRemote = true;
				}
			}
		}
		else {
			GCCVerifier.useLocalManifest();
		}

		if ( !offline && !loadedRemote && GCCVerifier.loadRemoteManifest() && GCCVerifier.isRemoteManifestNewer() ){
			while (true) {
				System.out.println("A manifest update is available! Download update?");
				System.out.println("0: No");
				System.out.println("1: Yes");
				System.out.print(">");
				Scanner s = new Scanner(System.in);
				int i = -1;
				try {
					i = s.nextInt();
				}
				catch ( java.util.InputMismatchException e ) {}
				if ( i < 0 || i > 1 ) {
					System.out.printf("Invalid selection.%n%n");
				}
				else if ( i == 0 ) {
					System.out.println("");
					break;
				}
				else {
					System.out.println("");
					GCCVerifier.saveRemoteManifestToLocal();
					GCCVerifier.useLocalManifest();
					break;
				}
			}
		}

		if ( !offline )
			GCCVerifier.updateLibFromManifest();

		//Main console loop
		while ( true ) {

			//Get the appropriate serial port
			String[] portNames = SerialPortList.getPortNames();
			String portName = "";
			if ( portNames.length == 0 ) {
				System.out.printf("No serial ports found. Check USB Connection...%n%n");
			}
			else {
				if ( portNames.length > 1 ) {
					while (true) {
						System.out.println("Please select a serial port: ");
						for ( int i = 0; i < portNames.length; i++ ) {
							System.out.printf("%d: %s%n", i, portNames[i]);
						}
						System.out.printf("%nPort>");
						Scanner s = new Scanner(System.in);
						int i = -1;
						try {
							i = s.nextInt();
						}
						catch ( java.util.InputMismatchException e ) {}
						if ( i < 0 || i > portNames.length - 1 ) {
							System.out.printf("Invalid selection.%n%n");
						}
						else {
							portName = portNames[i];
							System.out.println("");
							break;
						}
					}
				}
				else {
					System.out.printf("Serial port auto-selected.%n%n");
					portName = portNames[0];
				}

				//Choose the platform
				GCCVerifier.Platform platform;
				while (true) {
					GCCVerifier.Platform[] platformVals = GCCVerifier.Platform.values();
					if ( platformVals.length == 1 ) {
						platform = platformVals[0];
						System.out.printf("Platform auto-selected.%n%n");
						break;
					}
					System.out.println("Please select a platform: ");
					for ( int i = 0; i < platformVals.length; i++ ) {
						System.out.printf("%d: %s%n", i, platformVals[i].name());
					}
					System.out.printf("%nPlatform>");
					Scanner s = new Scanner(System.in);
					int i = -1;
					try {
						i = s.nextInt();
					}
					catch ( java.util.InputMismatchException e ) {}
					if ( i < 0 || i > platformVals.length - 1 ) {
						System.out.printf("%nInvalid selection. ");
					}
					else {
						platform = platformVals[i];
						System.out.println("");
						break;
					}
				}

				long start = System.currentTimeMillis();

				GCCVerifier verifier = new GCCVerifier(portName);
				verifier.selectPlatform(platform);

				GCCVerifier.VerifyParamsResult paramResult = verifier.verifyParams();
				boolean firmwareSuccess = verifier.verifyFirmwareImage();

				System.out.print(paramResult.output);
				System.out.println("--------------------------------");
				System.out.println("|  Firmware Mod Verification   |");
				System.out.println("--------------------------------");
				System.out.println("|                              |");
				if ( paramResult.succeeded ) {
					System.out.println("|                              |");
					System.out.println("|           SUCCESS!           |");
					System.out.println("|                              |");
				}
				else {
					System.out.println("|         ************         |");
					System.out.println("|         * FAILURE! *         |");
					System.out.println("|         ************         |");
				}
				System.out.println("|                              |");
				System.out.printf("--------------------------------%n%n");

				System.out.println("--------------------------------");
				System.out.println("|  Firmware Image Verification |");
				System.out.println("--------------------------------");
				System.out.println("|                              |");
				if ( firmwareSuccess ) {
					System.out.println("|                              |");
					System.out.println("|           SUCCESS!           |");
					System.out.println("|                              |");
				}
				else {
					System.out.println("|         ************         |");
					System.out.println("|         * FAILURE! *         |");
					System.out.println("|         ************         |");
				}
				System.out.println("|                              |");
				System.out.printf("--------------------------------%n%n");

				float elapsed = ((float) (System.currentTimeMillis() - start))/1000;
				System.out.printf("Verification took %.2fs.%n%n", elapsed);

			}

			while (true) {
				System.out.println("Verify another?");
				System.out.println("0: No");
				System.out.println("1: Yes");
				System.out.print(">");
				Scanner s = new Scanner(System.in);
				int i = -1;
				try {
					i = s.nextInt();
				}
				catch ( java.util.InputMismatchException e ) {}
				if ( i < 0 || i > 1 ) {
					System.out.printf("Invalid selection.%n%n");
				}
				else if ( i == 0 ) {
					System.exit(0);
				}
				else {
					break;
				}

			}

		} //main loop

	} //main()

} //class