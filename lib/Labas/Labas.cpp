#include "Labas.h"

Labas::Labas() {}

Labas::Labas(String username, String password, int attempts)
{
    this->username = username;
    this->password = password;
    this->attempts = attempts;
}

bool Labas::sendSMS(String recipient, String message)
{
    // bool successful = false;
    // for (int i = 0; !successful && i < this->attempts; i++)
    //     successful = this->login();

    // if (!successful)
    // {
    //     Serial.println("labas: failed to send sms");
    //     return false;
    // }

    String data =
        urlEncode("sms_submit[recipientNumber]") + '=' + urlEncode(recipient) + '&' +
        urlEncode("sms_submit[textMessage]") + '=' + urlEncode(message) + '&' +
        urlEncode("sms_submit[_token]") + '=' + urlEncode(this->token);

    HTTPClient http;
    if (!http.begin(baseURL))
    {
        Serial.print("labas: http: unable to parse: ");
        Serial.println(baseURL);
        return false;
    }
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Cookie", "scml=" + this->scml);

    http.POST(data);
    http.end();

    return true; // we just assume it went through.. for simplicity's sake
}

bool Labas::getSCML()
{
    HTTPClient http;

    if (!http.begin(loginRoute))
    {
        Serial.print("labas: http: unable to parse: ");
        Serial.println(loginRoute);
        return false;
    }
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Cookie", "scml=; TS011605d9=0");

    String data =
        "_username=" + urlEncode(this->username) +
        "&_password=" + urlEncode(this->password);

    int resCode = http.POST(data);
    http.end();
    if (resCode <= 0)
    {
        Serial.println("labas: unable to get scml");
        return false;
    }

    this->scml = this->substr(&http.rawHeaders, "scml=", ";");
    if (this->scml.length() == 0)
    {
        Serial.println("labas: unable to get scml");
        return false;
    }

    return true;
}

bool Labas::getToken()
{
    HTTPClient http;

    if (!http.begin(baseURL))
    {
        Serial.print("labas: http: unable to parse: ");
        Serial.println(baseURL);
        return false;
    }
    http.addHeader("Cookie", "scml=" + this->scml);

    int resCode = http.GET();
    if (resCode <= 0)
    {
        Serial.println("labas: http: unable to get page");
        http.end();
        return false;
    }

    String html = http.getString();
    http.end();

    int i = html.indexOf("name=\"sms_submit[_token]\"");
    if (i < 0)
    {
        Serial.println("labas: http: unable to get page");
        return false;
    }
    // html = html.substring(i);

    this->token = this->substr(&html, "value=\"", "\"", i);
    if (this->scml.length() == 0)
    {
        Serial.println("labas: unable to parse token");
        return false;
    }

    return true;
}

bool Labas::login()
{
    if (!getSCML())
        return false;

    if (!getToken())
        return false;

    return true;
}

String Labas::substr(String *string, const String &left, const String &right, int from)
{
    int a = string->indexOf(left, from);
    if (a < 0)
    {
        Serial.println("substr: unable to parse");
        return String();
    }
    a += left.length();

    int b = string->indexOf(right, a);
    if (b < 0)
    {
        Serial.println("substr: unable to parse");
        return String();
    }

    return string->substring(a, b);
}

String Labas::urlEncode(String str)
{
    String encodedString = "";
    char c;
    char code0;
    char code1;
    // char code2;
    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (c == ' ')
        {
            encodedString += '+';
        }
        else if (isalnum(c))
        {
            encodedString += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
            {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9)
            {
                code0 = c - 10 + 'A';
            }
            // code2 = '\0';
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
            //encodedString+=code2;
        }
        yield();
    }
    return encodedString;
}
