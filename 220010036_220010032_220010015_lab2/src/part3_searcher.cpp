#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <bits/stdc++.h>

using namespace std;

int main(int argc, char **argv)
{
	if(argc != 5)
	{
		cout <<"usage: ./partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position>\nprovided arguments:\n";
		for(int i = 0; i < argc; i++)
			cout << argv[i] << "\n";
		return -1;
	}
	
	char *file_to_search_in = argv[1];
	char *pattern_to_search_for = argv[2];
	int search_start_position = atoi(argv[3]);
	int search_end_position = atoi(argv[4]);
	
	string s(pattern_to_search_for);

	int arrsize = s.length();
	char array_search[arrsize];   // For comparing each character with the string that needs to be searched

	char arr[s.length() + 1];    // Array that stores the string to be searched
	strcpy(arr, s.c_str());

//	Opening the input text file
	std::ifstream file("file.txt");
	if (!file.is_open()) {
	        std::cerr << "Failed to open the file!" << std::endl;
	        return 1;
	    }

	char ch;
	int i=0, k=0, flag=0;
	int position;
	bool start = false;

//	Reading the input file character by character from the start position to the end position
	while (file.get(ch)) {
		if (i == search_start_position){
			start = true;
		}

		if (start == true){
			if (arr[k] == ch){
				if (k==arrsize-1){
//					cout<< i - arrsize+1 << endl;
					position = i - arrsize+1;
					flag=1;
				}

				array_search[k]=ch;
				k++;
			}
			else{
				k=0;
			}
		}

		if (i == search_end_position){
			break;
		}
		i++;
	}
	file.close();

//	Getting pid
	pid_t pid = getpid();
	if (flag == 1){
		cout << "["<<pid <<"]"<< " found at " << position << endl;
		
		return 1;
	}
	else{
//		cout << "["<<pid <<"]" << " didn't find" << endl;
		return 0;
	}


}
