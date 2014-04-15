float distance_between(float lat1, float long1, float lat2, float long2, float units_per_meter) {
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothised sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be upto 0.5%.
  float delta = radians(long1-long2);
  float sdlong = sin(delta);
  float cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  float slat1 = sin(lat1);
  float clat1 = cos(lat1);
  float slat2 = sin(lat2);
  float clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  float denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795 * units_per_meter;
}

uint32_t latitude, longitude;
char answer[9];

void setup(){
  Serial.begin(38400);}
  
void loop()
{
  latitude = 47367537;
  longitude = 122189949;

       
  int nondecimalLatitude = longitude/1000000;
  int decimalLatitude = (float(longitude - (longitude/1000000)*1000000))/60;
  String decimalLat = "";
  decimalLat += nondecimalLatitude;
  decimalLat += ".";
  decimalLat += decimalLatitude;
  Serial.println(decimalLat);
  
  for(int i = 0; i<9; i++) {answer[i] = decimalLat.charAt(i); }
  Serial.println(answer);
  float answer2 = atof(answer);
 Serial.println(answer2, 4); 
  delay(2000);
  float lat1 = 47.6133;
  float lat2 = 47.6233;
  float long1 = 122.3157;
  float long2 = 122.3133;
  Serial.println(distance_between(lat1, long1, lat2, long2, 1.0));
  delay(2000);
}
