#include <iostream>
#include <fstream>
#include <vector>

void generateTestData(std::vector<std::string> &filenames) {
  std::cout << "Wait.. Generating Test Data...\n";
  uint64_t filesize = 1000000;
  std::ofstream firstFile;
  firstFile.open(filenames.at(0));
  for (int i = 0; i < filesize; ++i) {
    firstFile << (uint8_t)rand(); 
  }
  firstFile.close();
  std::ifstream firstFileRead;
  firstFileRead.open(filenames.at(0));
  int times = 10;
  for (int i = 1; i < filenames.size(); ++i) {
    std::ofstream ofile;
    ofile.open(filenames.at(i)); 
    for (int j = 0; j < times; ++j) { 
      ofile << firstFileRead.rdbuf();
      firstFileRead.seekg(0, std::ios::beg);
    }
    times *= 10;    
    ofile.close();
  }
  std::cout << "Test data generated.";
}
