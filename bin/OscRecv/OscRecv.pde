/**
 * oscP5parsing by andreas schlegel
 * example shows how to parse incoming osc messages "by hand".
 * it is recommended to take a look at oscP5plug for an
 * alternative and more convenient way to parse messages.
 * oscP5 website at http://www.sojamo.de/oscP5
 */

import oscP5.*;
import netP5.*;

OscP5 oscP5;
NetAddress myRemoteLocation;
String info = "";

void setup() {
  size(400, 400);
  frameRate(25);
  oscP5 = new OscP5(this, 3333);
  myRemoteLocation = new NetAddress("127.0.0.1", 3000);
  //PFont font = createFont("宋体", 24);
  //textFont(font);
}

void mouseClicked() {
  OscMessage m = new OscMessage(mouseButton == LEFT ? "/start" : "/end");
  println("Clicked");
  oscP5.send(m, myRemoteLocation);
}

void draw() {
  background(0); 
  text("Click mouse:\nleft to send /start\nright to send /end\n" + info, 10, height/2);
}

void oscEvent(OscMessage m) {
  /* check if theOscMessage has the address pattern we are looking for. */
  info = "### received "+m.addrPattern() + " " + m.get(0).stringValue();
  println(info);
}