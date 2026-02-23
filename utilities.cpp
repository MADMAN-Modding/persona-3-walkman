#include "utilities.h"

String utilities::cleanText(String text) {
  text.replace("â€™", "'");  // Right single quote
  text.replace("â€˜", "'");  // Left single quote
  text.replace("â€œ", "\""); // Left double quote
  text.replace("â€", "\"");  // Right double quote
  text.replace("â€", "-");  // Em dash
  text.replace("â€", "-");  // En dash
  text.replace("Ã©", "e");
  text.replace("Ã¡", "a");
  text.replace("Ã³", "o");
  text.replace("Ã­", "i");
  text.replace("Ãº", "u");
  text.replace("：", ":");
  text.replace("’", "'");
  return text;
}