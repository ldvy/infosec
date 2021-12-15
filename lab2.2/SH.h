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
		"user <username> <block/unblock/delete>\tPerform the defined user operation\n"
		"help\tHelp message\n"
		"exit\tExit the shell\n";
}

string prompt(const string& workingDir) {
	if (whoami != "root")
		return whoami + "@localhost" + ":" + workingDir + "$ ";
	else
		return whoami + "@localhost" + ":" + workingDir + "# ";
}

bool authUser(Filesystem& fs, string username, const string& pass = "") {
	vector <string> tempVec = fs.getUserInfo(username);

	if (username == "guest") {
		whoami = username;
		fs.changeWorkingDir(tempVec[1]);
		tempVec.clear();
		return true;
	}

	if (username == "root") {
		if (pass == tempVec[1]) {
			fs.notifyRoot("/secrets/reg.jrn");
			whoami = username;
			fs.changeWorkingDir(tempVec[2]);
			tempVec.clear();
			return true;
		}
		else {
			cout << "Incorrect password for user " + username + "." << endl;
			tempVec.clear();
			//fs.fileAction("/secrets/reg.jrn", 0, "Failed log in attempt for user " + username + ".\n");  // action = 0 - append
			//fs.save();
			return false;
		}
	}

	else {
		if (stoi(tempVec[3]) == 0) {
			if (pass == tempVec[1]) {
				whoami = username;
				fs.changeWorkingDir(tempVec[2]);
				tempVec.clear();
				return true;
			}
			else {
				cout << "Incorrect password for user " + username + " or user doesn't exist." << endl;
				tempVec.clear();
				//fs.fileAction("/secrets/reg.jrn", 0, "Failed log in attempt for user " + username + ".\n");  // action = 0 - append
				//fs.save();
				return false;
			}
		}
		else {
			cout << "User " + username + " is banned. Please contact administrator." << endl;
			tempVec.clear();
			return false;
		}
	}
}

// --------------- LAB3 START
//.pw data structure username:password:homedir:isblocked(0 / 1):score:num_of_command_left:times_kicked:Q1:A1:Q2:A2:Q3:A3
bool kickBan(Filesystem& fs) {  // true for kick/ban needed, false for no kick/ban needed
	if (whoami != "root" and whoami != "guest") {
		vector <string> tempVec = fs.getUserInfo(whoami);

		if (stoi(tempVec[4]) == 0) {
			if (stoi(tempVec[6]) == 0) {
				cout << "User " + whoami + " got banned. Please contact administrator." << endl;
				fs.userOperations(whoami, 0);
				authUser(fs, "guest");
				return true;
			}
			else {
				cout << "User " + whoami + " got kicked. Please re-login again." << endl;
				fs.modifyUserData(whoami, 6, -1);  // Decreasing the leftover kicks number
				fs.modifyUserData(whoami, 4, -1);  // Decreasing user score
				authUser(fs, "guest");
				return true;
			}
		}
		else
			return false;
	}
	else
		return false;
}
// --------------- LAB3 END

bool execute(Filesystem& fs, const string& command, string workingDir) {
	istringstream iss(command);
	vector<string> params;
	copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(params));
	params.emplace_back("");
	params.emplace_back("");

	if (params[0] != "exit" and params[0] != "su") {
		if (!fs.askQuestion(whoami) and kickBan(fs))
			return true;
	}

	if (params[0] == "su") {
		if (params.size() == 4) {
			string input_pass;
			cout << "Enter password for " + params[1] + " (if any)" << endl;
			getline(cin, input_pass);

			if (authUser(fs, params[1], input_pass)) {
				cout << "Switched to " + params[1] + "." << endl;
			}
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "cp") {
		if (params.size() == 5) {
			if (fs.checkDirPermissions(whoami, params[1]) and fs.checkDirPermissions(whoami, params[2]))
				fs.copyFile(params[1], params[2]);
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
		else cout << "Excessive or insufficient operands." << endl;
	}

	else if (params[0] == "mv") {
		if (params.size() == 5) {
			if (fs.checkDirPermissions(whoami, params[1]) and fs.checkDirPermissions(whoami, params[2]))
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

	else if (params[0] == "user") {
		if (params.size() == 5) {
			if (whoami == "root") {
				if (params[1] != "root" and params[1] != "guest") {
					if (params[2] == "block")
						fs.userOperations(params[1], 0);
					else if (params[2] == "unblock")
						fs.userOperations(params[1], 1);
					else if (params[2] == "delete")                 // Currently user delete does not work
						fs.userOperations(params[1], 2);
					else
						cout << "Invalid user operation " + params[2] + "." << endl;
				}
				else cout << "Unable to perform operation " + params[2] + " on user " + params[1] + "." << endl;
			}
			else cout << "sh: " + command + " :Permission denied." << endl;
		}
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

	fs.modifyUserData(whoami, 5, -1);  // Decreasing the number of command before questioning

	return true;
}

#endif //FILE_SYSTEM_EMULATOR_SHELL_H