/**
 * Manifest.java is a helper class that handles loading and decoding of manifest
 * files.
 *
 * Contains the Manifest class and some necessary objects
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
import java.net.URL;
import java.io.*;
import java.nio.file.*;
import java.util.stream.Collectors;
import com.google.gson.*;

public class Manifest {
	public long timestamp = 0;
	public FirmwareModSpec[] modSpecs;
	public FirmwareImage[] firmwareImages;
	public static boolean debug = false;

	public Manifest() {}

	public void load(Path path) throws IOException {
		try {
			load(new String(Files.readAllBytes(path)));
		}
		catch ( NoSuchFileException e ) {
			System.out.printf("Could not find file %s.%n", path.toString());
			throw e;
		}
	}

	public void load(URL url) throws IOException {
		BufferedReader br = new BufferedReader(new InputStreamReader(url.openStream()));
		String json = br.lines().collect(Collectors.joining("\n"));
	 	br.close();
	 	load(json);
	}

	public void load(String json) throws JsonSyntaxException {
		if ( json.length() == 0 )
			throw new JsonSyntaxException("String is empty.");

		Gson gson = new GsonBuilder().setPrettyPrinting().create();
		Manifest temp = gson.fromJson(json, Manifest.class);
		timestamp = temp.timestamp;
		modSpecs = temp.modSpecs;
		firmwareImages = temp.firmwareImages;

		if ( debug )
			System.out.println(gson.toJson(this));

		if ( this.timestamp == 0 )
			throw new JsonSyntaxException("Manifest JSON did not contain timestamp.");
	}

	class FirmwareModSpec {
		public String name;
		public boolean permitted;
		public ModValueSpec[] valueSpecs = new ModValueSpec[]{};
	}

	class ModValueSpec {
		public String name;
		public int maxVal;
		public int minVal;
	}

	class FirmwareImage {
		public String name;
		public boolean permitted;
		public String hash;
		public int size;
		public String url;
	}
}

