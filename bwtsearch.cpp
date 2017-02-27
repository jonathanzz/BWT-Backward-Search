#include <iostream>
#include <string>
#include <fstream>
#include <cstdbool>
#include<vector>
#include<map>
#include <cstring>
#include <sstream>
#include <stack>
#define BLOCK_SIZE 1024

using namespace std;

//Function declaration.
bool commandArgumentHandling(int argc, char* argv[]);

void indexFileGenerate();

void cArrayGenerate();

void backwardSearch();

int findOcc(char c, unsigned int num, ifstream& bwt, ifstream& index);

void findOffset(unsigned int first, unsigned int last, ifstream& bwt, ifstream& index);

int readFromIndex(int block, char c, ifstream& index);

int noIndexOcc(char c, int num, ifstream& bwt);

void smallFileSearch();

//Global variable and container declaration
char* bwtFile;	//BWT file
char* indexFile;	//INDEX file
char* pattern;   //Search pattern

int option;   //-n or -a or -r?
unsigned int fileSize;   //file size of the bwt file
int blockNum;   //Total number of how many blocks.

vector<unsigned int> blockInWhichIndex; //Record the position of which block in which position in the index file.
unsigned int cArray[128] = { 0 }; //C Array
map<unsigned int, unsigned int> result; //Store the results when -a or -r is given.

int main(int argc, char* argv[]) {
	//Handling the commandline argument at the beginning.
	if (!commandArgumentHandling(argc, argv)) {
		cout << "Please check the format of commandline Argument" << endl;
		return 0;
	}
	ifstream ifIndex(indexFile);
	ifstream bwt;
	bwt.open(bwtFile, ios::in);
	bwt.seekg(0, ios::end);
	fileSize = bwt.tellg();
	blockNum = fileSize / BLOCK_SIZE;
	//For small size file(less than one block), directly search the result without generate index file.
	//Otherwise the index file would be larger then original file for very some small file.
	if (fileSize < BLOCK_SIZE) {
		smallFileSearch();
	} else {
		if (!ifIndex) {
			ifIndex.close();
			indexFileGenerate();
		} else {
			cArrayGenerate();
			ifIndex.close();
		}
		backwardSearch();
	}
	return 1;
}

//Deal with the commandline argument(etc.file path, -n, -a or -r?)
bool commandArgumentHandling(int argc, char* argv[]) {
	if (argc != 5)
		return false;
	string op = argv[1];
	if (op.length() != 2)
		return false;
	else if (op[0] != '-' || (op[1] != 'a' && op[1] != 'n' && op[1] != 'r'))
		return false;
	if (op[1] == 'n')
		option = 1;
	else
		option = op[1] == 'a' ? 2 : 3;

	bwtFile = argv[2];
	indexFile = argv[3];
	pattern = argv[4];
	return true;
}

//Small file search, no index file generate
void smallFileSearch() {
	ifstream bwt;
	map<char, unsigned int> countOccurs;
	bwt.open(bwtFile, ios::in);
	char ch = 0;
	//Calculate occurs time of each character.
	if (bwt.is_open()) {
		while (!bwt.eof()) {
			ch = bwt.get();
			countOccurs[(char) ch]++;
		}
	}
	bwt.close();
	//C array generate
	unsigned int sum = 0;
	sum += countOccurs[(char) 9];
	cArray[10] = sum;
	sum += countOccurs[(char) 10];
	cArray[13] = sum;
	sum += countOccurs[(char) 13];
	for (int i = 32; i < 127; i++) {
		cArray[i] = sum;
		sum += countOccurs[(char) i];
	}
	//Using backward search to find -n
	int nb = 0;
	int i = strlen(pattern) - 1;
	char c = pattern[i];
	int first = cArray[(int) c] + 1;
	int last = cArray[(int) c + 1];
	bwt.open(bwtFile, ios::in);
	while (first <= last && i >= 1) {
		c = pattern[i - 1];
		first = cArray[(int) c] + noIndexOcc(c, first - 1, bwt) + 1;
		last = cArray[(int) c] + noIndexOcc(c, last, bwt);
		i--;
	}
	if (first <= last)
		nb = last - first + 1;
	if (option == 1)
		cout << nb << endl;
	else {	//If -r or - a required.
		stack<int> numbers;
		int p = first - 1;
		//While loop to travers each searched result.
		while (p < last) {
			int t = p;
			bwt.seekg(t);
			char c = bwt.get();
			bool flag = false;
			//By using LF function to deduct back until [number] is find out.
			while (c != '[') {
				if (c == ']')
					flag = true;
				t = cArray[(int) c] + noIndexOcc(c, t, bwt);
				bwt.seekg(t);
				c = bwt.get();
				if (flag && c < 58 && c > 47)
					numbers.push((int) c - 48);
			}
			int num = 0;
			while (!numbers.empty()) {
				num = num * 10 + numbers.top();
				numbers.pop();
			}
			result[num]++;
			p++;
		}
		bwt.close();
		if (option == 2) {
			for (auto i = result.begin(); i != result.end(); i++) {
				cout << "[" << i->first << "]" << endl;
			}
		} else
			cout << result.size() << endl;
	}
}

//Find occ when no index file generated.
int noIndexOcc(char c, int num, ifstream& bwt) {
	int n = 0;
	bwt.seekg(ios::beg);
	for (int i = 0; i < num; i++) {
		if ((char) bwt.get() == c)
			n++;
	}
	return n;
}

//Generate the index file
void indexFileGenerate() {
	ifstream bwt;
	ofstream index;
	map<char, unsigned int> countOccurs;
	bwt.open(bwtFile, ios::in);
	bwt.seekg(0);
	index.open(indexFile);
	int offset = 0;
	int ch = 0;
	int block = 1;
	if (bwt.is_open()) {
		while (!bwt.eof()) {
			if (offset != BLOCK_SIZE) {
				ch = bwt.get();
				countOccurs[(char) ch]++;
				offset++;
			} else {
				if (countOccurs[(char) 9] == 0 && countOccurs[(char) 10] == 0
						&& countOccurs[(char) 13] == 0)
					index << '$';
				else {
					index << countOccurs[(char) 9] << " ";
					index << countOccurs[(char) 10] << " ";
					index << countOccurs[(char) 13] << " ";
				}
				index << '#';
				for (int i = 32; i < 127; i++)
					index << countOccurs[(char) i] << " ";
				index << "\n";
				offset = 0;
				block++;
			}
		}
		bwt.close();

		if (countOccurs[(char) 9] == 0 && countOccurs[(char) 10] == 0
				&& countOccurs[(char) 13] == 0)
			index << '$';
		else {
			index << countOccurs[(char) 9] << " ";
			index << countOccurs[(char) 10] << " ";
			index << countOccurs[(char) 13] << " ";
		}
		index << '#';
		for (int i = 32; i < 127; i++)
			index << countOccurs[(char) i] << " ";
		index << "\n";
		unsigned int sum = 0;
		index << sum << " ";
		sum += countOccurs[(char) 9];
		index << sum << " ";
		sum += countOccurs[(char) 10];
		index << sum << " ";
		sum += countOccurs[(char) 13];
		for (int i = 32; i < 127; i++) {
			index << sum << " ";
			sum += countOccurs[(char) i];
		}
	}
	index.close();
	cArrayGenerate();
}

//Generate the C array
void cArrayGenerate() {
	ifstream index;
	index.open(indexFile, ios::in);
	int cArrayInFile;
	string s;
	string temp;
	while (getline(index, temp)) {
		s = temp;
		cArrayInFile = (unsigned int) index.tellg();
		blockInWhichIndex.push_back(cArrayInFile);
	}
	istringstream iss(s);
	iss >> cArray[9];
	iss >> cArray[10];
	iss >> cArray[13];
	for (int i = 32; i < 127; i++)
		iss >> cArray[i];
	index.close();
}

//Start backward Search by using the algorithm given in lecture notes.
void backwardSearch() {
	ifstream index;
	index.open(indexFile, ios::in);
	ifstream bwt;
	bwt.open(bwtFile, ios::in);
	int nb = 0;
	int i = strlen(pattern) - 1;
	char c = pattern[i];
	unsigned int first = cArray[(int) c] + 1;
	unsigned int last = cArray[(int) c + 1];
	while (first <= last && i >= 1) {
		c = pattern[i - 1];
		first = cArray[(int) c] + findOcc(c, first - 1, bwt, index) + 1;
		last = cArray[(int) c] + findOcc(c, last, bwt, index);
		i--;
	}
	if (first <= last)
		nb = last - first + 1;
	if (option == 1)
		cout << nb << endl;
	else
		findOffset(first, last, bwt, index);
	index.close();
	bwt.close();
}

//Search the results when -n or -a is given.
void findOffset(unsigned int first,unsigned int last, ifstream& bwt, ifstream& index) {
	map<unsigned int, unsigned int> alreadyFindIndex;
	stack<int> numbers;
	unsigned int p = first - 1;
	//By using LF function to recall from the result character until [number] is found.
	while (p < last) {
		int times = 0;
		unsigned int t = p;
		bwt.seekg(t);
		char c = bwt.get();
		bool flag = false;
		bool al = true;
		while (c != '[') {
			//For over long record, store the position first time
			//and second time if this position being passed again, break the loop
			//because the record number has been stored already.
			if (times > 100) {
				if (alreadyFindIndex.find(t) != alreadyFindIndex.end()) {
					al = false;
					break;
				}
				alreadyFindIndex[t]++;
			}
			if (c == ']')
				flag = true;
			t = cArray[(int) c] + findOcc(c, t, bwt, index);
			bwt.seekg(t);
			c = bwt.get();
			times++;
			if (flag && c < 58 && c > 47)
				numbers.push((int) c - 48);

		}
		if (al) {
			int num = 0;
			while (!numbers.empty()) {
				num = num * 10 + numbers.top();
				numbers.pop();
			}
			result[num]++;
		} else {
			while (!numbers.empty())
				numbers.pop();
		}
		p++;
	}
	if (option == 2) {
		for (auto i = result.begin(); i != result.end(); i++) {
			cout << "[" << i->first << "]" << endl;
		}
	} else
		cout << result.size() << endl;
}

//Occ function.
//Find the nearest block position which recorded the occurs number in the index file
//then loop to the specific position and find the exact Occ number.
int findOcc(char c, unsigned int num, ifstream& bwt, ifstream& index) {
	bool fromBegin = true;
	int block = num / BLOCK_SIZE;
	unsigned int mid = block * BLOCK_SIZE + BLOCK_SIZE / 2;
	int searchLength;
	unsigned int from;
	if (num > mid) {
		from = block != blockNum ? (block + 1) * BLOCK_SIZE : fileSize;
		fromBegin = false;
		searchLength = from - num;
	} else {
		from = block * BLOCK_SIZE;
		searchLength = num - from;
	}

	int now =
			fromBegin ?
					readFromIndex(block - 1, c, index) :
					readFromIndex(block, c, index);
	char temp[searchLength];
	if (fromBegin) {
		bwt.seekg(from);
		bwt.read(temp, searchLength);
	} else {
		bwt.seekg(num);
		bwt.read(temp, searchLength);
	}

	for (int i = 0; i < searchLength; i++) {
		if (temp[i] == c) {
			if (fromBegin)
				now++;
			else
				now--;
		}
		if (temp[i] == EOF)
			break;
	}
	return now;
}

//Function to read index file when calculate the Occ number.
int readFromIndex(int block, char c, ifstream& index) {
	int now = 0;
	if (block < 0)
		return 0;
	else if (block > 0) {
		unsigned int num = blockInWhichIndex[block - 1];
		index.seekg(num);
	} else
		index.seekg(0);
	string s;
	getline(index, s);
	istringstream iss(s);
	if (s[0] != '$' && c < 31) {
		if (c == 9)
			iss >> now;
		else if (c == 10) {
			iss >> now;
			iss >> now;
		} else {
			iss >> now;
			iss >> now;
			iss >> now;
		}
	}
	string cut = s.substr(s.find('#') + 1);
	istringstream iscut(cut);
	for (int i = 32; i < 127; i++) {
		if (i == (int) c) {
			iscut >> now;
			break;
		}
		iscut >> now;
	}

	return now;
}

