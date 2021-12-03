#ifndef FILE_SYSTEM_EMULATOR_SHELL_H
#define FILE_SYSTEM_EMULATOR_SHELL_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "FS.h"

using namespace std;
string whoami = "guest";

string help() {
	return "rm <file/directory>\tRemove file or directory\n"
		"mkdir <directory>\tCreate directory\n"
		"touch <file>\tCreate an empty file\n"
		"vi <file>\tCreate a file with content\n"
		"ls <directory>\tPrint files and directories in the current path\n"
		"cd <directory>\tChange working directory\n"
		"mv <file/directory>\tMove file\n"
		"cat <file>\tPrint file contents\n"
		"stat <file>\tPrint file information\n"
		"adduser/useradd <username>\tCreate new user\n"
		"review <journal name.jrn>\tReview the specified journal file\n"
		"help\tHelp message\n"
		"exit\tExit the shell\n";
}

string prompt(const string& workingDir) {
	if (whoami != "root")
		return whoami + "@localhost" + ":" + workingDir + "$ ";
	else
		return whoami + "@localhost" + ":" + workingDir + "# ";
}

bool if_root(const string& uid) {
	if (uid == "root") return true;
	else return false;
}

bool authUser(Filesystem& fs, string username, const string& pass) {
	vector <string> tempVec = fs.getUserInfo(username);

	if (username == "guest") {
		whoami = username;
		fs.changeWorkingDir(tempVec[1]);
		tempVec.clear();
		return true;
	}
	else {
		if (pass == tempVec[1]) {
			if (username == "root")
				fs.notifyRoot("/secrets/reg.jrn");

			whoami = username;
			fs.changeWorkingDir(tempVec[2]);
			tempVec.clear();
			//fs.fileAction("/secrets/reg.jrn", 0, "Successful log in attempt for user " + username + ".\n");  // action = 0 - append
			//fs.save();
			return true;
		}
		else {
			tempVec.clear();
			//fs.fileAction("/secrets/reg.jrn", 0, "Failed log in attempt for user " + username + ".\n");  // action = 0 - append
			//fs.save();
			return false;
		}
	}
}

bool execute(Filesystem& fs, const string& command, string workingDir) {
	istringstream iss(command);
	vector<string> params;
	copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(params));
	params.emplace_back("");
	params.emplace_back("");

	if (params[0] == "su") {
		if (params.size() == 4) {
			string input_pass;
			cout << "Enter password for " + params[1] + " (if any)" << endl;
			getline(cin, input_pass);

			if (authUser(fs, params[1], input_pass)) {
				cout << "Switched to " + params[1] + "." << endl;
			}
			else
				cout << "Access denied or user doesn't exist." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "cp") {
		if (params.size() == 5) {
			if (fs.checkDirPermissions(whoami, params[1]))
				fs.copyFile(params[1], params[2]);
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "mv") {
		if (params.size() == 5) {
			if (fs.checkDirPermissions(whoami, params[1]))
				fs.moveFile(params[1], params[2]);
			else
				cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "rm") {
		if (params.size() == 4) {
			if (fs.checkDirPermissions(whoami, params[1]))
				fs.deleteFile(params[1]);
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "mkdir") {
		if (params.size() == 4) {
			if (fs.checkDirPermissions(whoami, params[1]))
				fs.createDir(params[1]);
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "touch") {
		if (params.size() == 4) {
			if (fs.checkDirPermissions(whoami, params[1])) {
				int size = 0;
				try { size = stoi(params[1]); }
				catch (const exception& e) {};
				fs.createFile(params[1], size * 1024);
			}
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "vi" or params[0] == "vim") {
		if (params.size() == 4) {
			if (fs.checkDirPermissions(whoami, params[1])) {
				int size = 1;
				try { size = stoi(params[1]); }
				catch (const exception& e) {};
				fs.createFile(params[1], size * 1024, true);
			}
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "stat") {
		if (params.size() == 4) {
			if (fs.checkDirPermissions(whoami, params[1])) {
				fs.showFileStatus(params[1]);
			}
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "adduser" or params[0] == "useradd") {
		if (params.size() == 4) {
			if (whoami == "root") {
				fs.createUser(params[1]);
			}
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "review") {
		if (params.size() == 4) {
			if (whoami == "root") {
				fs.notifyRoot("/secrets/" + params[1]);
			}
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "cat") {
		if (params.size() == 4) {
			if (fs.checkDirPermissions(whoami, params[1]))
				cout << fs.printFile(params[1]) << endl;
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "ls") {
		if (params.size() == 3)
			cout << fs.list(workingDir) << endl;
		else if (params.size() == 4)
			cout << fs.list(params[1]) << endl;
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "pwd") {
		if (params.size() == 3)
			cout << fs.getWorkingDir() << endl;
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "cd") {
		if (params.size() == 4)
			fs.changeWorkingDir(params[1]);
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "help") {
		if (params.size() == 3)
			cout << help() << endl;
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "exit") {
		if (params.size() == 3) {
			fs.exit();
			exit(0);
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else {
		cout << "sh: " + command + " :Command not found." << endl;
	}
	return true;
}

#endif //FILE_SYSTEM_EMULATOR_SHELL_H