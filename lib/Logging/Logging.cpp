#include "Logging.h"
// https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_spiffs_storage.htm

Logging::Logging(fs::FS *fs, const char *dirPath, int files, int perFile)
{
	this->fs = fs;
	this->name = (char *)dirPath;
	this->maxFiles = files;
	this->maxPerFile = perFile;

	this->n = (this->getFilesCount() > 0 ? this->getFileLinesCount(0) : 0);
}

bool Logging::log(const char *line)
{
	if (this->n >= this->maxPerFile)
		if (!this->rotate())
			return false;

	if (!this->append(0, line))
		return false;

	this->n++;
	return true;
}

String Logging::getPath(int log)
{
	String path = this->name;
	path += '/';
	path += log;
	return path;
}

int Logging::getFilesCount()
{
	String path = this->name;

	File root = this->fs->open(path);
	if (!root)
	{
		return 0;
	}

	if (!root.isDirectory())
	{
		Serial.print(path);
		Serial.println(F(": not a directory"));
		root.close();
		return -1;
	}

	int n = 0;
	for (File file = root.openNextFile(); file; file = root.openNextFile())
	{
		file.close();
		n++;
	}

	root.close();
	return n;
}

int Logging::getFileLinesCount(int log)
{
	String path = this->getPath(log);

	File file = this->fs->open(path);
	if (!file)
	{
		Serial.print(path);
		Serial.println(F(": failed to open file for reading"));
		return -1;
	}

	if (file.isDirectory())
	{
		Serial.print(path);
		Serial.println(F(": not a file"));
		file.close();
		return -1;
	}

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

bool Logging::append(int log, const char *line)
{
	String path = this->getPath(log);

	File file = this->fs->open(path, FILE_APPEND);
	if (!file)
	{
		Serial.print(path);
		Serial.println(F(": failed to open file for appending"));
		return false;
	}

	if (!file.print(line))
	{
		file.close();
		Serial.print(path);
		Serial.println(F(": failed to append a file"));
		return false;
	}

	if (!file.print('\n'))
	{
		file.close();
		Serial.print(path);
		Serial.println(F(": failed to append a file"));
		return false;
	}

	file.close();
	return true;
}

bool Logging::rotate()
{
	int fc = this->getFilesCount();

	String path;
	for (int i = this->maxFiles - 1; i < fc; i++)
	{
		path = this->getPath(i);
		if (!this->fs->remove(path))
		{
			Serial.print(path);
			Serial.println(F(": failed to remove file"));
			return false;
		}
	}

	if (fc > this->maxFiles - 1)
		fc = this->maxFiles - 1;

	String from;
	path = this->getPath(fc);
	for (int i = fc - 1; i >= 0; i--)
	{
		from = this->getPath(i);
		if (!this->fs->rename(from, path))
		{
			Serial.print(from);
			Serial.print(F(": failed to rename to "));
			Serial.println(path);
			return false;
		}
		path = from;
	}

	this->n = 0;
	return true;
}
