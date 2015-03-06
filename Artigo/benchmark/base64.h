//#define FALSE 0
//#define TRUE 1
//
//static const char* base64_chars =
//             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//             "abcdefghijklmnopqrstuvwxyz"
//             "0123456789+/";
//
//static int is_base64(unsigned char c);
//
//int find_next(const char *buffer, int start, char obj) {
//	int i, rc = FALSE;
//	for (i = start; i < strlen(buffer); i++) {
//		if (buffer[i] == obj) {
//			rc = i + 1;
//			break;
//		}
//	}
//	return rc;
//}
//
//
//char find_base64 (char c){
//	int i = 0;
//	while (c != base64_chars[i]){
//		i++;
//	}
//	return (char)i;
//}
//
//static int is_base64(unsigned char c) {
//  return (isalnum(c) || (c == '+') || (c == '/'));
//}
//
//unsigned char * base64_decode(char *encoded_string, int *length) {
//  int in_len = strlen(encoded_string);
//  int i = 0;
//  int j = 0;
//  int in_ = 0;
//  unsigned char char_array_4[4], char_array_3[3];
//  unsigned char *ret;
//  *length = 0;
//
//  ret = (unsigned char *)calloc(in_len, sizeof(unsigned char));
//
//  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
//    char_array_4[i++] = encoded_string[in_]; in_++;
//    if (i ==4) {
//      for (i = 0; i <4; i++)
//        char_array_4[i] = find_base64(char_array_4[i]);
//
//      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
//      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
//      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
//
//      for (i = 0; (i < 3); i++){
//		sprintf((char *)ret, "%s%c", ret, char_array_3[i]);
//		(*length)++;
//      }
//      i = 0;
//    }
//  }
//
//  if (i) {
//    for (j = i; j <4; j++)
//      char_array_4[j] = 0;
//
//    for (j = 0; j <4; j++)
//      char_array_4[j] = find_base64(char_array_4[j]);
//
//    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
//    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
//    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
//
//    for (j = 0; (j < i - 1); j++){
//    		sprintf((char *)ret, "%s%c", ret, char_array_3[j]);
//    		(*length)++;
//    }
//  }
//
//  return ret;
//}
//
//char * base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
//  char *ret;
//  int i = 0;
//  int j = 0;
//  unsigned char char_array_3[3];
//  unsigned char char_array_4[4];
//
//	ret = (char *)calloc(in_len + in_len, sizeof(char));
//
//  while (in_len--) {
//    char_array_3[i++] = *(bytes_to_encode++);
//    if (i == 3) {
//      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
//      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
//      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
//      char_array_4[3] = char_array_3[2] & 0x3f;
//
//      for(i = 0; (i <4) ; i++)
//        sprintf(ret, "%s%c", ret, base64_chars[char_array_4[i]]);
//      i = 0;
//    }
//  }
//
//  if (i)
//  {
//    for(j = i; j < 3; j++)
//      char_array_3[j] = '\0';
//
//    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
//    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
//    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
//    char_array_4[3] = char_array_3[2] & 0x3f;
//
//    for (j = 0; (j < i + 1); j++)
//      sprintf(ret, "%s%c", ret, base64_chars[char_array_4[j]]);
//
//    while((i++ < 3))
//		sprintf(ret, "%s%c", ret, '=');
//
//  }
//  return ret;
//}
//
