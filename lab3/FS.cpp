#include "FS.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory.h>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <utility>

#include <sstream>
#include <vector>


using namespace std;

// Creating data volume
Filesystem::Filesystem() {
	fsFilename = "data";
	workingDir = "/home/guest/";
	if (!load()) {
		cerr << "Fatal error: data load failed." << endl;
		exit();
	}
	bitmap = reinterpret_cast <bitset < BITMAP_SIZE > *> (memory + INODE_NUM * INODE_SIZE);
}

void Filesystem::initialize() {
	string plain_pass;
	unsigned int dumpy;
	createDir(dumpy);
}

bool Filesystem::load() {
	memory = (char*)malloc(FS_SIZE);
	if (memory == nullptr) return false;
	ifstream input(fsFilename);
	if (input) { // File exists.
		input.seekg(0, ios::end);
		int length = input.tellg();
		if (length != FS_SIZE) cerr << "Warning: the file size doesn't match." << endl;
		input.seekg(0, ios::beg);
		input.read(memory, length);
		input.close();
		bitmap = reinterpret_cast <bitset < BITMAP_SIZE > *> (memory + INODE_NUM * INODE_SIZE);
	}
	else { // File doesn't exist.
		memset(memory, 0, FS_SIZE);
		initialize();
		bitmap = reinterpret_cast <bitset < BITMAP_SIZE > *> (memory + INODE_NUM * INODE_SIZE);
		createDir("/home");
		createDir("/secrets");
		createFile("/secrets/reg.jrn", 1024, false);
		save();
		createUser("root");
		createUser("guest");
	}
	return true;
}

bool Filesystem::save() {
	ofstream output(fsFilename, ios::binary | ios::trunc);
	output.write(memory, FS_SIZE);
	return true;
}

bool Filesystem::exit() {
	save();
	free(memory);
	memory = nullptr;
	return true;
}

bool Filesystem::deleteFile(string path) {
	path = fullPath(path);
	if (path == "/") {
		cerr << "rm: Permission denied." << endl;
		return false;
	}
	else if (path == workingDir) {
		cerr << "rm: Cannot delete working dir." << endl;
		return false;
	}
	if (!exist(path)) {
		cerr << "rm: Cannot remove '" << path << "': No such file or directory." << endl;
		return false;
	}
	vector < string > names = splitPath(path);
	unsigned int parentDirInodeNumber = inodeNumber(names[0]);
	string name = names[1];
	unsigned int inodeNum = inodeNumber(name, parentDirInodeNumber);
	if (revokeInode(inodeNum)) {
		Inode* parent = readInode(parentDirInodeNumber);
		unsigned int& blockAddress = parent->address[0];
		char* buffer = readBlock(blockAddress);
		auto* dirItem = new DirItem;
		int targetPos;

		int occurences = count(path.begin(), path.end(), '/') - 1;
		cout << occurences << endl;

		for (targetPos = 0; targetPos * DIR_ITEM_SIZE < parent->size; ++targetPos) {
			dirItem = (DirItem*)(buffer + targetPos * DIR_ITEM_SIZE);
			string temp(dirItem->name);
			if (temp == name) {
				targetPos -= occurences;
				break;
			}
		}

		memmove(buffer + parentDirInodeNumber * INODE_SIZE + targetPos * DIR_ITEM_SIZE,
			buffer + parentDirInodeNumber * INODE_SIZE + (targetPos + 1) * DIR_ITEM_SIZE,
			//counter * DIR_ITEM_SIZE - (targetPos + 1) * DIR_ITEM_SIZE);
			parent->size - targetPos * DIR_ITEM_SIZE);

		parent->size -= DIR_ITEM_SIZE;
		save();

		return true;
	}
	return false;
}

bool Filesystem::revokeInode(unsigned int inodeNum) {
	// If this inode is a directory, we also need to delete all files that it include.
	Inode* inode = readInode(inodeNum);
	// TODO: When deleting directory, recursively delete all content in it.
	bool isDir = inode->type == '1';
	auto addresses = blockAddress(inode);
	for (auto address : addresses) {
		revokeBlock(address);
	}
	memset(memory + inodeNum * INODE_SIZE, 0, INODE_SIZE);
	return true;
}

bool Filesystem::createDir(string path) {
	path = fullPath(path);
	if (exist(path)) {
		cerr << "mkdir: Cannot create directory '" << path << "': Directory already exists." << endl;
		return false;
	}
	vector < string > names = splitPath(path);
	unsigned int parentDirInodeNum = inodeNumber(names[0]);
	string dirName = names[1];
	unsigned int inodeNum;
	if (createDir(inodeNum)) {
		return addDirItem(parentDirInodeNum, inodeNum, dirName);
	}
	return false;
}

bool Filesystem::createDir(unsigned int& inodeNum) {
	if (assignInode(inodeNum)) {
		Inode inode = createInode(true);
		writeInode(inodeNum, &inode);
		return true;
		save();
	}
	return false;
}

unsigned int Filesystem::inodeNumber(string path) {
	path = fullPath(path);
	auto names = parsePath(path);
	unsigned int targetInodeNumber = 0;
	for (const string& name : names) {
		targetInodeNumber = inodeNumber(name, targetInodeNumber);
	}
	return targetInodeNumber;
}

unsigned int Filesystem::inodeNumber(const string& name, unsigned int dirInodeNumber) {
	Inode* parent = readInode(dirInodeNumber);
	unsigned int& blockAddress = parent->address[0];
	char* buffer = readBlock(blockAddress);
	DirItem* dirItem;
	for (int i = 0; i * DIR_ITEM_SIZE < parent->size; ++i) {
		dirItem = (DirItem*)(buffer + i * DIR_ITEM_SIZE);
		string temp(dirItem->name);
		if (temp == name) {
			return dirItem->inodeNum;
		}
	}
	return 0;
}

bool Filesystem::assignInode(unsigned int& inodeNum) {
	Inode* temp;
	for (int i = 0; i < INODE_NUM; ++i) {
		temp = readInode(i);
		if (temp->type == 0) {
			inodeNum = i;
			return true;
		}
	}
	cerr << "No more iNodes available." << endl;
	return false;
}

Inode Filesystem::createInode(bool isDir) {
	Inode inode{};
	inode.createTime = time(nullptr);
	inode.type = isDir ? '1' : '2';
	return inode;
}

void Filesystem::writeInode(unsigned int inodeNumber, Inode* inode) {
	char* src = (char*)inode;
	char* dis = memory + inodeNumber * INODE_SIZE;
	memcpy(dis, src, INODE_SIZE);
}

Inode* Filesystem::readInode(unsigned int inodeNumber) {
	return (Inode*)(memory + inodeNumber * INODE_SIZE);
}

void Filesystem::writeBlock(unsigned int address, char* buffer) {
	char* dis = memory + BLOCK_START_POS + address * BLOCK_SIZE;
	memcpy(dis, buffer, BLOCK_SIZE);
}

void Filesystem::fillBlock(unsigned int address, bool empty) {
	const int num = BLOCK_SIZE / sizeof(char);
	char buffer[num];
	if (empty)
		memset(buffer, 0, num);

	writeBlock(address, buffer);
}

char* Filesystem::readBlock(unsigned int address) {
	return memory + BLOCK_START_POS + address * BLOCK_SIZE;
}

bool Filesystem::assignBlock(unsigned int& blockNum) {
	// We need 0 to tell us whether the address is used.
	for (int i = 1; i < BITMAP_SIZE; ++i) {
		if (!(*bitmap)[i]) {
			blockNum = i;
			(*bitmap)[blockNum] = true;
			return true;
		}
	}
	cerr << "Failed to assign block." << endl;
	return false;
}

bool Filesystem::revokeBlock(unsigned int& blockNum) {
	if (blockNum >= BITMAP_SIZE) return false;
	(*bitmap)[blockNum] = false;
	return true;
}

vector < string > Filesystem::parsePath(const string& path) {
	vector < string > names;
	string temp;
	for (char c : path) {
		if (c != '/') {
			temp += c;
		}
		else {
			if (!temp.empty()) {
				names.emplace_back(temp);
				temp = "";
			}
		}
	}
	if (!temp.empty()) names.emplace_back(temp);
	return names;
}

vector < string > Filesystem::splitPath(const string& path) {
	vector < string > names;
	int splitIndex = 0;
	string temp;
	for (int i = path.size() - 1; i >= 0; --i) {
		if (path[i] != '/') {
			temp += path[i];
		}
		else {
			if (!temp.empty()) {
				splitIndex = i;
				break;
			}
		}
	}
	names.push_back(path.substr(0, splitIndex));
	if (path.substr(splitIndex)[0] == '/') {
		names.push_back(path.substr(splitIndex + 1));
	}
	else {
		names.push_back(path.substr(splitIndex));
	}
	if (names[0].empty()) names[0] = "/";
	return names;
}

bool Filesystem::showFileStatus(string path) {
	path = fullPath(path);
	unsigned int inodeNum = inodeNumber(path);
	if (inodeNum == 0 && path != "/") {
		cout << "stat: Cannot stat '" << path << "': No such file or directory." << endl;
		return false;
	}
	Inode* inode = readInode(inodeNum);
	cout << "File: " << path << endl <<
		"Inode: " << inodeNum << endl <<
		"Size: " << inode->size << " B" << endl <<
		"Type: " << (inode->type == '1' ? "directory" : "regular file") << endl <<
		"Create time: " << inode->createTime << endl <<
		"Block address: ";
	auto addresses = blockAddress(inode);
	for (auto address : addresses) {
		cout << address << " ";
	}
	cout << endl;
	return true;
}

bool Filesystem::exist(const string& path) {
	unsigned int inodeNum = inodeNumber(path);
	return !(inodeNum == 0 && path != "/");
}

bool Filesystem::existPath(const string& path) {
	unsigned int inodeNum = inodeNumber(path);
	Inode* inode = readInode(inodeNum);
	return (inodeNum != 0 || path == "/") && (inode->type == '1');
}

string Filesystem::fullPath(const string& path) {
	if (path.empty()) {
		cerr << "pwd: Empty path specified." << endl;
		return "";
	}
	if (path[0] == '/') return path;
	if (workingDir.front() != '/') workingDir = '/' + workingDir;
	if (workingDir.back() != '/') workingDir += '/';
	return workingDir + path;
}

vector < unsigned > Filesystem::blockAddress(Inode* inode) {
	vector < unsigned > addresses;
	int addressNum = inode->size / BLOCK_SIZE;
	if (addressNum * BLOCK_SIZE < inode->size) addressNum++;
	for (int i = 0; i < min(addressNum, DIRECT_ADDRESS_NUM); ++i) {
		addresses.push_back(inode->address[i]);
	}
	if (addressNum > DIRECT_ADDRESS_NUM) {
		//auto *indirectAddresses = reinterpret_cast<unsigned int *>(readBlock(addresses[DIRECT_ADDRESS_NUM]));
		auto* buffer = new unsigned[addressNum - DIRECT_ADDRESS_NUM];
		memcpy(buffer, readBlock(inode->address[DIRECT_ADDRESS_NUM]),
			(addressNum - DIRECT_ADDRESS_NUM) * sizeof(unsigned));
		for (int i = 0; i + DIRECT_ADDRESS_NUM < addressNum; ++i) {
			addresses.push_back(buffer[i]);
		}
		delete[] buffer;
	}
	return addresses;
}

bool Filesystem::addDirItem(unsigned parentDirInodeNum, unsigned fileInodeNum,
	const string& name) {
	Inode* inode = readInode(parentDirInodeNum);
	unsigned int& blockAddress = inode->address[0];
	if (blockAddress == 0 && !assignBlock(blockAddress)) {
		cerr << "Failed to assign block." << endl;
		return false;
	}
	char* buffer = readBlock(blockAddress);
	auto* dirItem = new DirItem;
	dirItem->inodeNum = fileInodeNum;
	strncpy_s(dirItem->name, name.c_str(), MAX_FILENAME_LENGTH);
	memcpy(buffer + inode->size, dirItem, DIR_ITEM_SIZE);
	inode->size += DIR_ITEM_SIZE;
	delete dirItem;
	return true;
}

bool Filesystem::createFile(string path, int size, bool edit) {
	if (size < 0) size = 0;
	unsigned restSpace = (BLOCK_NUM - bitmap->count()) * BLOCK_SIZE;
	if (size > restSpace) {
		cerr << "mkdir: Not enough free space." << endl;
		size = 0;
	}

	if (size > MAX_FILE_SIZE) {
		cerr << "mkdir: Max file size is set to " << MAX_FILE_SIZE << " bytes." << endl;
		size = MAX_FILE_SIZE;
	}
	path = fullPath(path);
	if (exist(path)) {
		cerr << "touch: Cannot create file '" << path << "': File already exists." << endl;
		return false;
	}
	vector < string > names = splitPath(path);
	unsigned int parentDirInodeNum = inodeNumber(names[0]);
	string fileName = names[1];
	unsigned int inodeNum;
	if (createFile(inodeNum, size, edit)) {
		return addDirItem(parentDirInodeNum, inodeNum, fileName);
		save();
	}
	return false;
}

void Filesystem::editFile(unsigned int blockAddress, string text) {
	string file;
	string row;

	if (text == "") {
		while (getline(cin, row)) {
			if (row == ":wq") break;
			file += row + "\n";
		}
	}
	else
		file += text;

	char* dis = memory + BLOCK_START_POS + blockAddress * BLOCK_SIZE;
	int size = min(BLOCK_SIZE, (int)file.size());
	memcpy(dis, file.c_str(), size);
	save();
}

bool Filesystem::changeWorkingDir(string path) {
	path = fullPath(path);
	if (!existPath(path)) {
		cerr << "cd: No such directory." << path << endl;
		return false;
	}
	workingDir = path;
	if (workingDir[0] != '/') {
		workingDir = '/' + workingDir;
	}
	return true;
}

string Filesystem::getWorkingDir() {
	return workingDir;
}

string Filesystem::list(string path) {
	stringstream content;

	if (path.empty()) {
		path = workingDir;
	}
	else {
		path = fullPath(path);
	}
	unsigned dirInodeNumber = inodeNumber(path);
	if (dirInodeNumber == 0 && path != "/") {
		return "ls: Cannot access '" + path + "': No such file or directory.\n";
	}
	Inode* parent = readInode(dirInodeNumber);
	unsigned int& blockAddress = parent->address[0];
	char* buffer = readBlock(blockAddress);
	content << std::left << setw(5) << "INode" << " Name " << endl;
	DirItem* dirItem;

	for (int i = 0; i * DIR_ITEM_SIZE < parent->size; ++i) {
		dirItem = (DirItem*)(buffer + i * DIR_ITEM_SIZE);
		content << std::left << setw(5) << dirItem->inodeNum << " " << dirItem->name << endl;
	}
	return content.str();
}

bool Filesystem::copyFile(string sourceFilePath, string targetFilePath) {
	unsigned srcInodeNum = inodeNumber(std::move(sourceFilePath));
	Inode* srcInode = readInode(srcInodeNum);
	createFile(targetFilePath, srcInode->size);
	unsigned dstInodeNum = inodeNumber(std::move(targetFilePath));
	Inode* dstInode = readInode(dstInodeNum);
	auto srcAddresses = blockAddress(srcInode);
	auto dstAddresses = blockAddress(dstInode);
	for (int i = 0; i < srcAddresses.size(); ++i) {
		char* realSrcAddress = readBlock(srcAddresses[i]);
		char* realDstAddress = readBlock(dstAddresses[i]);
		memcpy(realDstAddress, realSrcAddress, BLOCK_SIZE);
	}
	return true;
}

bool Filesystem::moveFile(const string& sourceFilePath, string targetFilePath) {
	copyFile(sourceFilePath, std::move(targetFilePath));
	deleteFile(sourceFilePath);
	return true;
}

bool Filesystem::checkDirPermissions(string username, string wantedPath) {
	int dirParameterPos;

	if (username == "guest")
		dirParameterPos = 1;
	else
		dirParameterPos = 2;

	wantedPath = fullPath(wantedPath);
	if (wantedPath.find(getUserInfo(username)[dirParameterPos]) != std::string::npos)
		return true;
	else
		return false;
}

bool Filesystem::createFile(unsigned int& inodeNum, unsigned int size, bool edit, string username) {
	if (assignInode(inodeNum)) {
		Inode inode = createInode(false);
		inode.size = size;
		int blockNum = size / BLOCK_SIZE;
		if (blockNum * BLOCK_SIZE < size) blockNum++;
		for (int i = 0; i <= min(blockNum, DIRECT_ADDRESS_NUM); ++i) {
			assignBlock(inode.address[i]);
			fillBlock(inode.address[i], edit);
			if (i == 0 && edit) {
				if (username != "")
					defineUserInfo(inode.address[i], username);
				else
					editFile(inode.address[i]);
			}
		}
		if (blockNum >= DIRECT_ADDRESS_NUM) {
			assignBlock(inode.address[DIRECT_ADDRESS_NUM]);
			auto* blockAddresses = new unsigned[blockNum - DIRECT_ADDRESS_NUM];
			for (int i = 0; i <= blockNum - DIRECT_ADDRESS_NUM; ++i) {
				assignBlock(blockAddresses[i]);
				fillBlock(blockAddresses[i], edit);
			}
			// Write the extra block address into the last direct block
			writeBlock(inode.address[DIRECT_ADDRESS_NUM], reinterpret_cast <char*> (blockAddresses));
			delete[] blockAddresses;
		}
		writeInode(inodeNum, &inode);
		return true;
	}
	return false;
}

bool Filesystem::createUser(string username) {
	if (username != "root")
		createDir("/home/" + username);
	save();

	string userFilePath = "/secrets/" + username + ".pw";

	unsigned fileInodeNum = inodeNumber(std::move("/secrets/" + username + ".pw"));
	if (fileInodeNum != 0) {
		cerr << "User already exists." << endl;
		return false;
	}

	vector < string > names = splitPath(userFilePath);
	unsigned int parentDirInodeNum = inodeNumber(names[0]);
	string fileName = names[1];
	unsigned int inodeNum;
	if (createFile(inodeNum, 1024, true, username)) {
		return addDirItem(parentDirInodeNum, inodeNum, fileName);
	}
	return false;
}

//.pw data structure username:password:homedir:isblocked(0 / 1):score:num_of_command_left:times_kicked:Q1:A1:Q2:A2:Q3:A3
void Filesystem::defineUserInfo(unsigned int blockAddress, string username) {
	// PLUS username
	string content = username;
	string password;
	string tempString = "";

	if (username == "guest")
		content += ":/home/guest";
	else {
		cout << "Define " + username + " password:\n";
		while (tempString == "") {
			getline(cin, tempString);
			password = tempString;
		}

		// PLUS password AND home directory
		if (username == "root")
			content += ":" + tempString + ":" + "/";
		else {
			content += ":" + tempString + ":" + "/home/" + username;

			// PLUS isBlocked (0 = false) AND failed question attempts AND num of commands left AND number of kicks before ban
			content += ":0:" + to_string(USER_SCORE) + ":" + to_string(COMMANDS_LEFT) + ":" + to_string(KICKS_LEFT);

			// PLUS secret questions
			for (int i = 1; i <= 3; i++) {
				tempString = "";
				cout << "\nEnter secret question " << i << ":\n";
				while (tempString == "") {
					getline(cin, tempString);
				}
				content += ":" + tempString;

				tempString = "";
				cout << "Enter the answer for question " << i << ":\n";
				while (tempString == "") {
					getline(cin, tempString);
				}
				content += ":" + tempString;
			}
		}
	}

	// user.pw file creation
	content += '\0';
	char* dis = memory + BLOCK_START_POS + blockAddress * BLOCK_SIZE;
	int size = min(BLOCK_SIZE, (int)content.size());
	memcpy(dis, content.c_str(), size);

	// notify reg journal
	if (username != "guest")
		fileAction("/secrets/reg.jrn", 0, "User " + username + " registered with password - " + password + ".\n");  // action = 0 - append
	save();
}

string Filesystem::printFile(string path) {
	string content = "";
	unsigned fileInodeNum = inodeNumber(std::move(path));
	if (fileInodeNum == 0 && path != "/") {
		cerr << "Cannot access '" << path << "': No such file or directory." << endl;
		return "";
	}
	Inode* inode = readInode(fileInodeNum);
	char* buffer = new char[inode->size];
	auto addresses = blockAddress(inode);
	char* dstAddress = buffer;
	for (auto address : addresses) {
		char* srcAddress = readBlock(address);
		memcpy(dstAddress, srcAddress, BLOCK_SIZE);
		dstAddress += BLOCK_SIZE;
	}
	for (int i = 0, counter = 0; i < inode->size; ++i, ++counter) {
		if (buffer[i] == '\0') break;
		content += buffer[i];
		if (counter == 63)
			counter = 0;
	}

	while (content.back() == '\n')
		content.pop_back();
	while (content.front() == '\n')
		content.erase(content.begin());

	delete[] buffer;
	return content;
}

bool Filesystem::fileAction(string path, int action, string text) {  //action = 0 - append to the end; action = 1 - pop first element; action = 2 - clear file.
	string fileContent = "";

	unsigned fileInodeNum = inodeNumber(std::move(path));
	if (fileInodeNum == 0) {
		cout << "File does not exist." << endl;
		return false;
	}

	Inode* inode = readInode(fileInodeNum);
	char* buffer = new char[inode->size];
	auto addresses = blockAddress(inode);
	char* dstAddress = buffer;

	for (auto address : addresses) {
		char* srcAddress = readBlock(address);
		memcpy(dstAddress, srcAddress, BLOCK_SIZE);
		dstAddress += BLOCK_SIZE;
	}
	for (int i = 0, counter = 0; i < inode->size; ++i, ++counter) {
		if (buffer[i] == '\0') break;
		fileContent += buffer[i];
		if (counter == 63) counter = 0;
	}
	delete[] buffer;

	if (action == 0) {
		editFile(addresses[0], fileContent + text);
		save();
	}
	else if (action == 1) {
		clearFile(addresses[0]);
		editFile(addresses[0], text);
		save();
	}
	else if (action == 2) {
		clearFile(addresses[0]);
		save();
	}

	return true;
}

void Filesystem::clearFile(unsigned int blockAddress) {
	char* dis = memory + BLOCK_START_POS + blockAddress * BLOCK_SIZE;
	memset(dis, '\0', BLOCK_SIZE);
	save();
}

vector<string> Filesystem::getUserInfo(string username) {
	string path = "/secrets/" + username + ".pw";
	string tempStr = printFile(path);

	if (tempStr == "")
		return { "USERNOTFOUND", "USERNOTFOUND", "USERNOTFOUND", "", "", "", "", "", "", "", "", "" };

	std::vector<std::string> strings;
	std::istringstream f(tempStr);
	std::string s;
	while (std::getline(f, s, ':')) {
		strings.push_back(s);
	}
	return strings;
}

bool Filesystem::notifyRoot(string path) {
	string events = printFile(path);

	if (events != "") {
		cout << "\nPlease review the below events that took place while you were away:\n" << events << "\n\n";
		return fileAction("/secrets/reg.jrn", 2);  // action = 0 - clean
	}
	else
		cout << "\nThere are no events in journal " + path << ".\n\n";

	return false;
}


// --------------- LAB3 START
string vectorToString(vector <string> input) {
	string output = "";

	for (string elem : input)
		output += elem + ":";
	output.pop_back();

	return output;
}

void Filesystem::userOperations(string username, int operation) {  // operation=0 - block, operation=1 - unblock, operation=2 - delete user 
	vector<string> userFileContent = getUserInfo(username);

	if (operation == 0) {
		userFileContent[3] = "1";
		userFileContent[4] = "0";
		userFileContent[5] = "0";
		userFileContent[6] = "0";
		fileAction("/secrets/" + username + ".pw", 2);
		fileAction("/secrets/" + username + ".pw", 0, vectorToString(userFileContent));
	}

	if (operation == 1) {
		userFileContent[3] = "0";
		userFileContent[4] = to_string(USER_SCORE);
		userFileContent[5] = to_string(COMMANDS_LEFT);
		userFileContent[6] = to_string(KICKS_LEFT);
		fileAction("/secrets/" + username + ".pw", 2);
		fileAction("/secrets/" + username + ".pw", 0, vectorToString(userFileContent));
	}

	if (operation == 2)
		deleteFile("/secrets/" + username + ".pw") and deleteFile("/home/" + username);
}

//.pw data structure username:password:homedir:isblocked(0 / 1):score:num_of_command_left:times_kicked:Q1:A1:Q2:A2:Q3:A3
void Filesystem::modifyUserData(string username, int position, int scoreIncrement) {
	if (username != "root" and username != "guest") {
		vector<string> userFileContent = getUserInfo(username);

		if ((stoi(userFileContent[position]) + scoreIncrement) <= 0)
			userFileContent[position] = to_string(0);
		else {
			if (position == 4 and (stoi(userFileContent[position]) + scoreIncrement) >= USER_SCORE)
				userFileContent[position] = to_string(USER_SCORE);
			else
				userFileContent[position] = to_string(stoi(userFileContent[position]) + scoreIncrement);
		}

		fileAction("/secrets/" + username + ".pw", 2);
		fileAction("/secrets/" + username + ".pw", 0, vectorToString(userFileContent));
	}
}

// Question indexes 7, 9, 11
bool Filesystem::askQuestion(string username) {
	if (username != "root" and username != "guest") {
		vector<string> userFileContent = getUserInfo(username);
		vector <string> questions = { userFileContent[7], userFileContent[9], userFileContent[11] };
		vector <string> answers = { userFileContent[8], userFileContent[10], userFileContent[12] };
		int randIndex = rand() % 3;

		if (stoi(userFileContent[5]) == 0) {  // if no commands left
			string input = "";

			cout << "Input answer for question:\n" + questions[randIndex] << endl;

			while (input == "") {
				getline(cin, input);
				if (input == answers[randIndex]) {
					modifyUserData(username, 5, COMMANDS_LEFT);  // num of commands left = max
					modifyUserData(username, 4, 1);  // increase score by 1
					cout << "Correct answer, number of command increased to " << COMMANDS_LEFT << ". Score increased by 1." << endl;
					return true;
				}
				else {
					modifyUserData(username, 4, -1); // decrease score by 1
					modifyUserData(username, 5, -COMMANDS_LEFT);  // num of commands left = 0
					cout << "Incorrect answer, user score decreased by 1." << endl;
					return false;
				}
			}
		}
		userFileContent.clear();
		return true;
	}
	return true;
}
// --------------- LAB3 END