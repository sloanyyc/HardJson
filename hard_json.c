#include <stdio.h>
#include <string.h>

// 读取JSON字符串的某个key值,允许直接读取多层json内的值
// 1.考虑到全字匹配，需要传入\"key\", 或者\"key(对于多层json)
// 2.返回值是dst的结束后空间
// 3.end可以为0表示默认(")，多层json可以用}或}\"结尾，数字应该为
// 4.若dst==js,则忽略拷贝字符串到dst，若len为0则直接修改js字符串0结束.不为0则不修改,返回长度
// 5.dend缓冲区结束指针，0忽略
// 5.使用范例
// char buf[1024] = {0};
// char* dst = buf, *dend = buf+(sizeof(buf));
// char* js = `{"a":"s1","b":1,"c":"{\"d\":\"s2\",\"e\":3.14}","e":0}`;
// 
static char* read_json_val(char** var, char*dst, char* dend, char* js, char* key, char* end, int* len)
{
  int jslen = 0;
  char ch = 0;
  char* tmp = 0;
  char* tmp1 = 0;
  if (!dst || !js || !key || !var) {
    return 0;
  }
  ch = dst==js?-1:0;
  jslen = strlen(js);
  dend = dend?dend:(dst+jslen);
  js = strstr(js, key);
  if (!js) {
    return 0;
  }
  js += strlen(key);
  while (js[0] && js[0]!='\\' && js[0] != '"') {
    js++;
  }
  js = strstr(js, ":");
  if (!js) {
    return 0;
  }
  js++;
  if (end && end[0] == ',') {
    // number
    tmp = strstr(js, end);
    if (!tmp) {
      tmp = strstr(js, "}");
    } else {
      tmp1 = (char*)memchr(js, '}', tmp-js);
      if (tmp1 && tmp1 < tmp) {
        tmp = tmp1;
      }
    }
  } else {
    // string
    tmp = strstr(js, "\"");
    if (tmp) {
      js = tmp+1;
      tmp = strstr(js, end?end:"\"");
    }
    while (tmp && tmp[-1] == '\\') {
      tmp = tmp-1;
    }
	if (tmp[0] == '}' && end && end[0] == '}') {
		tmp++;
	}
  }
  if (!tmp) {
    return 0;
  }
  // tmp is end of value
  if (ch == -1) {
    // no copy
    *var = js;
    if (len) {
      *len = tmp-js;
    } else {
      tmp[0] = 0;
    }
  } else {
    // copy to dst
    memcpy(dst, js, tmp-js);
    dst[tmp-js] = 0;
    *var = dst;
    dst = dst + (tmp-js) + 1;
  }
  return dst;
}

int unquote_json(char* cb)
{
	int i = 0, j = 0;
	while (cb[i]) {
		if (cb[i] == '\\') {
			cb[j++] = cb[++i];
		}
		else {
			cb[j++] = cb[i];
		}
		i++;
	}
	cb[j] = 0;
	return j;
}

#define HJF_FIRST           0x00000001
#define HJF_LAST            0x00000002
#define HJF_OBJ_START       0x00000004
#define HJF_OBJ_END         0x00000008


char* append_json_val(char* dst, const char* key, const char* val, int flag)
{
  int i = 0, j = 0;
  sprintf(dst, "%s\"%s\":\"", (flag&HJF_OBJ_START?"{":""), key);
  dst = dst+strlen(dst);
  while (val[i]) {
    if (val[i] == '\\' || val[i] == '"') {
      dst[j++] = '\\';
      dst[j++] = val[i];
    }
	else {
      dst[j++] = val[i];
    }
    i++;
  }
  dst[j++] = '"';
  if (!(flag&HJF_LAST)) {
    dst[j++] = ',';
  }
  if (flag&HJF_OBJ_END) {
    dst[j++] = '}';
  }
  dst[j] = 0;
  return dst+j;
}

int main(void) {
  int len = 0;
  char buf[1024] = {0};
  char* dst = buf, *dend = buf+(sizeof(buf));
  char* json = 0;
  char* js = "{\"a\":\"s1\",\"b\":1,\"ls\":\"long string.\",\"c\":\"{\\\"d\\\":\\\"s2\\\",\\\"e\\\":3.14}\",\"ls\":\"second long string.\",\"f\":0}";
  char* a = 0, *b = 0, *c = 0, *d = 0, *e = 0, *f = 0, *ls = 0, *ls2 = 0;
  dst = read_json_val(&a, dst, 0, js, "a", 0, 0);
  dst = read_json_val(&b, dst, 0, js, "b", ",", 0);
  dst = read_json_val(&c, dst, 0, js, "c", "}", 0);
  dst = read_json_val(&d, dst, 0, js, "d", 0, 0);
  dst = read_json_val(&e, dst, 0, js, "e", ",", 0);
  dst = read_json_val(&f, dst, 0, js, "f", ",", 0);
  // check one time dst == 0 no effect
  if (!dst) {
    printf("read json failed!\n");
  }
  unquote_json(c);
  // pass dst == js , just pointer ls to right pos
  js = read_json_val(&ls, js, 0, js, "ls", 0, &len);
  memcpy(dst, ls, len);
  dst[len] = 0;
  json = dst;
  dst += len+1;
  // exp: read second key ls
  dst = read_json_val(&ls2, dst, 0, ls, "ls", 0, &len);
  ls = json;
  printf("js %s\na = %s\nb = %s\nc = %s\nd = %s\ne = %s\nf = %s\nls = %s\nls2 = %s\n", js, a, b, c, d, e, f, ls, ls2);

  json = dst;
  dst = append_json_val(dst, "a", a, HJF_OBJ_START);
  dst = append_json_val(dst, "b", b, 0);
  dst = append_json_val(dst, "ls", ls, 0);
  dst = append_json_val(dst, "c", c, 0);
  // dst = append_json_val(dst, "d", d, 0);
  // dst = append_json_val(dst, "e", e, 0);
  dst = append_json_val(dst, "ls", ls2, 0);
  dst = append_json_val(dst, "f", f, HJF_OBJ_END|HJF_LAST);
  printf("json %s\n", json);
  return 0;
}
