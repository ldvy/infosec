#include <iostream>
#include <string>
#include "FS.h"
#include "SH.h"

using namespace std;

int main() {
	Filesystem fs;
	string input;

	while (true) {
		cout << prompt(fs.getWorkingDir());
		getline(cin, input);
		if (input.empty()) continue;
		if (!execute(fs, input, fs.getWorkingDir())) break;
	}
	return 0;
}