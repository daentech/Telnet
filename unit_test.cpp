#include "unit_test.h"

bool isParentDirectory(string child, string parent){

    // Remove any trailing slashes
    if(child.c_str()[child.length()] == '/'){
        child = child.substr(0, child.length() - 1);
    }

    int slash_pos = child.find_last_of("/");
    child = child.substr(0, slash_pos);
    
    cout << child << endl;
    cout << parent << endl;
    
    if(child.compare(parent) == 0) return true;

    return false;
}
