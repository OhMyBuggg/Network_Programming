#include<bits/stdc++.h>

using namespace std;

void reverse(string &word){
	string output = "";
	int len = word.length();
	for(int i=len-1; i>=0; i--){
		output = output + word[i];
	}
	cout << output << endl;
}

void split(string &word, string &cut){
	char *words = new char[word.length() + 1];
	strcpy(words, word.c_str());

	char *cuts = new char[cut.length() + 1];
	strcpy(cuts, cut.c_str());

	char *pch = strtok(words, cuts);
	while(pch != NULL){
		cout << pch << " ";
		pch = strtok(NULL, cuts);
	}
	cout << endl;
}

int main(int argc, char *argv[]){
	ifstream inputFile;
	inputFile.open(argv[1], ifstream::in);
	string line;
	string cut = argv[2];
	int filelen = strlen(argv[1]);

	cout << "-------------------Input file "<< argv[1] << "-----------------" << endl;

	while(getline(inputFile, line)){
		string word;
		string command;
		istringstream iss(line);
		iss >> command;
		istringstream iss2(line);
		while(iss2 >> word){
			cout << word << " ";
		}
		cout << endl;
		if(command == "reverse"){
			reverse(word);
		}else if(command == "split"){
			split(word, cut);
		}
	}

	cout << "-------------------End of input file " << argv[1] << "----------" << endl;
	cout << "*******************User input******************";
	for(int i=0; i<filelen; i++){
		cout << "*";
	}
	cout << endl;

	string input;
	while(getline(cin, input)){
		istringstream iss3(input);
		string userCommand;
		string userWord;
		iss3 >> userCommand;
		if(userCommand == "exit"){
			return 0;
		}else if(userCommand == "reverse"){
			while(iss3 >> userWord){
				reverse(userWord);
			}
		}else if(userCommand == "split"){
			while(iss3 >> userWord){
				split(userWord, cut);
			}
		}
	}
}