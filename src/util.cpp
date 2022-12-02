#include <Arduino.h>


char const *strs = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
String randStr(int length){
    String randStr = "";
    for (int i = 0; i < 20; i++) randStr += strs[random(62)];
    return randStr;
}