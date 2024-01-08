/*
 *  Copyright (C)2023, VadRov, all right reserved.
 *
 *  Проект демонстрирует работу с виртуальным com портом.
 *  Воспроизведение стримингового видео (motion jpeg)
 *
 *	Серверная часть
 *  Данная программа в реальном времени непрерывно делает скриншоты экрана,
 *  масштабирует их до заданных размеров и кодирует в формат изображения jpeg.
 *  Полученные закодированные изображения передаются по виртуальному COM-порту
 *  (USB) клиенту-MCU, на стороне которого изображения декодируются из jpeg формата
 *  и выводятся на дисплей. Таким образом, изображение на мониторе компьютера
 *  дублируется на дисплее, подключенном к микроконтроллеру, выполняющему
 *  функции клиента, подключенного к серверу (компьютеру) посредством USB.
 *  Сервер путем изменения качества кодирования, начиная с некоторого заданного 
 *  максимального значения PICTURE_JPEG_QUALITY пытается подстроить размер выходного
 *  jpeg файла под запросы клиента. Минимальное качество кодирования ограничено значением
 *  5.
 *
 *  Допускается свободное распространение.
 *  При любом способе распространения указание автора ОБЯЗАТЕЛЬНО.
 *  В случае внесения изменений и распространения модификаций указание первоначального автора ОБЯЗАТЕЛЬНО.
 *  Распространяется по типу "как есть", то есть использование осуществляется на свой страх и риск.
 *  Автор не предоставляет никаких гарантий.
 
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 *
 *
 *
 *  Используемый JPEG encoder:
 *  jpeg-compressor
 *  Public Domain or Apache 2.0, Richard Geldreich <richgel99@gmail.com>
 *  https://github.com/richgel999/jpeg-compressor
 * 
 */

#include <iostream>
#include <regex>
#include <windows.h>
#include <winuser.h>
#include "jpge.h"

using namespace std;

//Максимальное качество при кодировании jpeg.
//Именно с этого значения будет стартовать декодер,
//чтобы уложиться в максимальный размер сообщения для MCU (PICTURE_MAX_SIZE)
#define PICTURE_JPEG_QUALITY	70 

int PICTURE_WIDTH = 0, PICTURE_HEIGHT = 0;	//разрешение дисплея (эти параметры передаст MCU)
int PICTURE_MAX_SIZE = 0;					//максимальный размер сообщения, которое готов принять MCU

BYTE* pixels = 0, * jpeg_file = 0;

HDC hDisplay, hDC;
HBITMAP hBitmap;
HGDIOBJ hGDI;
BITMAPINFOHEADER bmi = { 0 };

static void CaptureScreen(void);
static int EncodeToJPEG(int);
static void exit_prog(void);
static void ShowCOMPorts(void);

int main(void)
{
	setlocale(LC_ALL, "Russian"); //Включение поддержки русского языка в консольном приложении

	cout << "Список доступных COM портов:" << endl;
	ShowCOMPorts();

	WCHAR port_name[10];
	cout << "Укажите из этого списка тот порт, к которому подключено устройство: ";
	wcin.getline(port_name, sizeof(port_name) - 1);

	HANDLE port = CreateFile(port_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (port == INVALID_HANDLE_VALUE) {
		cout << "Ошибка открытия порта!" << endl;
		exit_prog();
	}

	//Параметры соединения (Можно игнорировать. Здесь они не играют никакой роли).
	//Скорость передачи определяется реализацией USB в м/к: HS - до 480 Мбит/с, FS - до 12 Мбит/с.
	//Используя стандартные библиотеки от производителей м/к (ST, Artery), для USB FS автору удавалось получить 
	//скорость приема/передачи до 4.5 Мбит/с.
	DCB params = { 0 };
	params.DCBlength = sizeof(DCB);
	params.BaudRate = CBR_115200;
	params.ByteSize = 8;
	params.StopBits = ONESTOPBIT;
	params.Parity = NOPARITY;

	if (!SetCommState(port, &params)) {
		cout << "Ошибка конфигурирования порта!" << endl;
		CloseHandle(port);
		exit_prog();
	}

	//Настройка таймаутов
	COMMTIMEOUTS commtimeouts = { min((1000 + params.BaudRate - 1) / params.BaudRate, 1), 0, 0, 0, 0 };
	if (!SetCommTimeouts(port, &commtimeouts)) {
		cout << "Ошибка установки таймаутов порта!" << endl;
		CloseHandle(port);
		exit_prog();
	}

	DWORD b_write, b_read = 0;
	BYTE buf_in[100];
	int buf_size = 0;
	BOOL res;
	//Ожидание и получение сообщения от клиента-MCU
	while (1) {
		if (!ReadFile(port, &buf_in[buf_size], 1, &b_read, 0)) {
			cout << "Ошибка чтения данных из порта!" << endl;
			CloseHandle(port);
			exit_prog();
		}
		if (!b_read) break;
		buf_size += b_read;
	}
	buf_in[buf_size] = '\0';
	//Преобразование сообщения из char* в string
	string str((char*)buf_in);
	//Шаблон, которому должно соответствовать сообщение клиента-MCU
	//Этому шаблону соответствует сообщение вида: MCU:240x320.F:12000.
	//Означающее,что разрешение дисплея у м/к 240х320 и микроконтроллер готов 
	//принимать сообшения/файлы размером до 12000 байт
	regex r("MCU:[ ]{0,}(\\d{1,})[ ]{0,}X[ ]{0,}(\\d{1,})[ ]{0,}\\.F:[ ]{0,}(\\d{1,})[ ]{0,}\\.");
	smatch m;
	//Проверка сообщения на соответствие заданному шаблону и выделение из него параметров 
	//разрешения графического дисплея клиента-MCU
	if (!regex_search(str, m, r)) {
		cout << "Получен некорректный ответ от MCU!" << endl;
		CloseHandle(port);
		exit_prog();
	}

	//Получение ширины и высоты графического дисплея и максимального размера сообщения 
	//клиента-MCU c преобразование к типу int
	PICTURE_WIDTH = stoi(m[1].str());
	PICTURE_HEIGHT = stoi(m[2].str());
	PICTURE_MAX_SIZE = stoi(m[3].str());

	cout << "Клиент-MCU сообщил следующие сведения: " << endl;
	cout << "Разрешение графического дисплея клиента-MCU " << PICTURE_WIDTH << "X" << PICTURE_HEIGHT << endl;
	cout << "Клиент-MCU готов принимать сообщения размером до "<< PICTURE_MAX_SIZE << " байт" << endl;

	pixels = new BYTE[3 * PICTURE_WIDTH * PICTURE_HEIGHT];
	if (3 * PICTURE_WIDTH * PICTURE_HEIGHT > 1024) {
		jpeg_file = new BYTE[3 * PICTURE_WIDTH * PICTURE_HEIGHT];
	}
	else {
		jpeg_file = new BYTE[1024];
	}

	//Получаем контекст экрана.
	hDisplay = GetDC(0);
	//Создаем контекст устройства памяти, совместимый с контекстом экрана.
	hDC = CreateCompatibleDC(hDisplay);
	//Cоздаем растровое изображение, совместимое с растром на экране, но с другим разрешением.
	hBitmap = CreateCompatibleBitmap(hDisplay, PICTURE_WIDTH, PICTURE_HEIGHT);
	//Выбираем в качестве объекта растровое изображение в контексте ранее созданного совместимого с экраном устройства.
	hGDI = SelectObject(hDC, hBitmap);

	//Определяем формат изображения специальной структурой
	bmi.biSize = sizeof(BITMAPINFOHEADER); //Размер этой структуры в байтах.
	bmi.biPlanes = 1;					   //Количество плоскостей устройства (всегда 1).
	bmi.biBitCount = 24;				   //Количество битов на пиксель. 24 бита (3 байта) на точку, 2^24 цветов.
	bmi.biWidth = PICTURE_WIDTH;		   //Ширина растрового изображения в пикселях.
	bmi.biHeight = -PICTURE_HEIGHT;		   //Высота растрового изображения в пикселях. 
	//Если значение biHeight положительное, то растровое изображение является 
	//diB снизу вверх, а его источником является левый нижний угол. 
	//Если значение biHeight отрицательное, то растровое изображение представляет
	//собой diB сверху вниз, а его источником является левый верхний угол.
	bmi.biCompression = BI_RGB;			   //Несжатый формат изображения (без кодирования).
	bmi.biSizeImage = 0;				   //Размер изображения в байтах. Для формата BI_RGB может быть равно 0.

	DWORD bytes_all = 0;

	//Собственно, "бескончный цикл", в котором делается скриншот, кодируется в jpeg и отправляется MCU
	while (1) {
		CaptureScreen(); //делаем скриншот
		//Пробуем сжимать изображение за несколько попыток, изменяя качество кодирования,
		//чтобы уложиться в максимальный размер сообщения для MCU
		for (int quality = PICTURE_JPEG_QUALITY > 5 ? PICTURE_JPEG_QUALITY: 5; quality >= 5; quality -= 5) {
			buf_size = EncodeToJPEG(quality);
			if (buf_size <= PICTURE_MAX_SIZE) break;
		}
		//Отправляем данные в порт
		res = WriteFile(port, jpeg_file, buf_size, &b_write, 0);
		if (!res || b_write < (uint32_t)buf_size) {
			cout << endl << "Ошибка передачи данных в порт!" << endl;
			break;
		}
		bytes_all += buf_size; //счетчик переданных клиенту-MCU данных
		cout << "Передано (в кадре/всего) " << buf_size << "/" << bytes_all << " байт                \r";
	}
	CloseHandle(port);
	exit_prog();
}

//Копируем экран в картинку (массив цветовых составляющих rgb)
static inline void CaptureScreen(void)
{
	//Получаем разрешение экрана.
	int w = GetDeviceCaps(hDisplay, HORZRES);
	int h = GetDeviceCaps(hDisplay, VERTRES);

	//Определяем режим растяжения\сжатия изображения
	SetStretchBltMode(hDC, COLORONCOLOR/*HALFTONE или COLORONCOLOR*/); //HALFTONE - медленно, но четче при уменьшении.
	//Копируем с масштабированием (растяжение\сжатие) изображение из контекста экрана в контекст созданного совместимого устройства
	BOOL bRet = StretchBlt(hDC, 0, 0, PICTURE_WIDTH, PICTURE_HEIGHT, hDisplay, 0, 0, w, h, SRCCOPY);
	//Извлекаем биты совместимого растрового изображения и копируем их в буфер в виде DIB с использованием заданного формата bmi
	GetDIBits(hDC, hBitmap, 0, PICTURE_HEIGHT, pixels, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

	//Меняем местами компоненты цвета r и b (после GetDIBits BGR, а станет RGB)
	for (int i = 0, tmp; i < 3 * PICTURE_WIDTH * PICTURE_HEIGHT; i += 3) {
		tmp = pixels[i + 2];
		pixels[i + 2] = pixels[i];
		pixels[i] = tmp;
	}
}

//Собственно, название процедуры говорит само за себя
//Кодирование в Jpeg с заданным качеством
static inline int EncodeToJPEG(int quality)
{
	int buf_size = 3 * PICTURE_WIDTH * PICTURE_HEIGHT;
	if (buf_size < 1024) buf_size = 1024;

	jpge::params params;
	params.m_quality = quality;
	params.m_subsampling = jpge::H2V2;	/*
										 * Y_ONLY - grayscale
										 * H1V1 - YCbCr, no subsampling (YCbCr 1x1x1, 3 blocks per MCU)
										 * H2V1 - YCbCr subsampling (YCbCr 2x1x1, 4 blocks per MCU)
										 * H2V2 - YCbCr subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
										 */
	params.m_two_pass_flag = true;
	params.m_use_std_tables = true;
	jpge::compress_image_to_jpeg_file_in_memory(jpeg_file, buf_size, PICTURE_WIDTH, PICTURE_HEIGHT, 3, pixels, params);
	return buf_size;
}

static void exit_prog(void)
{
	SelectObject(hDC, hGDI);
	DeleteDC(hDC);
	ReleaseDC(0, hDisplay);
	DeleteObject(hBitmap);

	delete[] pixels;
	delete[] jpeg_file;
	system("Pause");
	exit(1);
}

//Выводит список доступных COM-портов
//Берет их из системного реестра Windows
static void ShowCOMPorts(void)
{
	int r = 0;
	HKEY hkey = 0;
	//Открываем раздел реестра, в котором хранится иинформация о COM портах
	r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM\\", 0, KEY_READ, &hkey);
	if (r != ERROR_SUCCESS) {
		return;
	}
	DWORD CountValues, MaxValueNameLen, MaxValueLen;
	//Получаем информацию об открытом разделе реестра
	RegQueryInfoKey(hkey, 0, 0, 0, 0, 0, 0, &CountValues, &MaxValueNameLen, &MaxValueLen, 0, 0);
	MaxValueNameLen++;
	//Выделяем память
	TCHAR* bufferName = 0, * bufferData = 0;
	bufferName = new TCHAR[MaxValueNameLen];
	if (!bufferName) {
		RegCloseKey(hkey);
		return;
	}
	bufferData = new TCHAR[MaxValueLen + 1];
	if (!bufferData) {
		delete[] bufferName;
		RegCloseKey(hkey);
		return;
	}

	DWORD i, NameLen, type, DataLen;
	//Цикл перебора параметров раздела реестра
	for (i = 0; i < CountValues; i++) {
		NameLen = MaxValueNameLen;
		DataLen = MaxValueLen;
		r = RegEnumValue(hkey, i, bufferName, &NameLen, 0, &type, (LPBYTE)bufferData, &DataLen);
		if ((r != ERROR_SUCCESS) || (type != REG_SZ)) 	{
			continue;
		}
		wcout << bufferData << endl;
	}
	//Освобождаем память
	delete[] bufferName;
	delete[] bufferData;
	//Закрываем раздел реестра
	RegCloseKey(hkey);
}