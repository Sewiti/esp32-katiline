#include "Logging.h"
// https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

Logging::Logging(fs::FS *fs, const char *path, int keep_soft, int keep_hard)
{
	this->fs = fs;
	this->path = (char*) path;
	this->keep_soft = keep_soft;
	this->keep_hard = keep_hard;

	this->n = this->getCount();

	char tempPath[strlen(this->path) + sizeof TEMP_SUFFIX];
	sprintf(tempPath, "%s" TEMP_SUFFIX, this->path);

	if (this->fs->exists(tempPath))
		this->fs->remove(tempPath);
}

bool Logging::log(const char *line)
{
	if (this->n < this->keep_hard)
	{
		if (!this->append(line))
			return false;
		this->n++;
		return true;
	}

	if (!this->rotateAppend(line))
		return false;
	this->n = this->keep_soft;
	return true;
}

int Logging::getCount()
{
	File file = this->fs->open(this->path);
	if (!file)
		return 0;

	int n = 0;
	while (file.available())
	{
		n++;
		if (!file.find('\n'))
			break;
	}

	file.close();
	return n;
}

bool Logging::append(const char *line)
{
	File file = this->fs->open(this->path, FILE_APPEND);
	if (!file)
	{
		Serial.print(path);
		Serial.println(F(": failed to open file for appending"));
		return false;
	}

	if (!file.println(line))
	{
		Serial.print(path);
		Serial.println(F(": failed to append a file"));
		file.close();
		return false;
	}

	file.close();
	return true;
}

bool Logging::rotateAppend(const char *line)
{
	char tempPath[strlen(this->path) + sizeof TEMP_SUFFIX];
	sprintf(tempPath, "%s" TEMP_SUFFIX, this->path);

	File file = this->fs->open(this->path, FILE_READ);
	if (!file)
	{
		Serial.print(path);
		Serial.println(F(": failed to open file for reading"));
		return false;
	}

	// skip first lines
	for (int i = this->keep_soft; i <= this->n; i++)
	{
		if (!file.find('\n'))
		{
			Serial.print(path);
			Serial.println(F(": endline not found"));
			file.close();
			return false;
		}
	}
	String content = file.readString();
	file.close();

	File tempFile = this->fs->open(tempPath, FILE_WRITE);
	if (!tempFile)
	{
		Serial.print(tempPath);
		Serial.println(F(": failed to open file for writing"));
		return false;
	}

	if (!tempFile.print(content))
	{
		Serial.print(tempPath);
		Serial.println(F(": failed to write"));
		tempFile.close();
		return false;
	}

	// append line
	if (!tempFile.println(line))
	{
		Serial.print(tempPath);
		Serial.println(F(": failed to write"));
		tempFile.close();
		return false;
	}
	tempFile.flush();
	tempFile.close();

	// replace with temp file
	if (!fs->remove(this->path)) {
		Serial.print(this->path);
		Serial.println(F(": unable to remove"));
		return false;
	}

	if (!fs->rename(tempPath, this->path))
	{
		Serial.print(tempPath);
		Serial.print(F(": unable to rename to "));
		Serial.println(this->path);
		return false;
	}
	return true;
}
