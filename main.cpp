#include <iostream>

#include <vector>

#include <stack>

#include <unordered_map>

#include <fstream>

#include <sstream>

#include <algorithm>

#include <string>

#include <memory>

#include <limits>

#include <list>

using namespace std;

extern int currentTime;
int currentTime = 0;

struct Process {
  int process_id;
  int total_pages;
  vector < int > page_access_sequence;
  stack < int > page_stack;
  unordered_map < int, int > page_table;
  int fault_count = 0;

  Process(): process_id(0), total_pages(0), fault_count(0) {}
};

class PageReplacementStrategy {
  public: virtual void handlePageFault(Process & proc, int page) = 0;
  virtual~PageReplacementStrategy() {}
};

class WorkingSetStrategy: public PageReplacementStrategy {
  private: int windowSize;
  unordered_map < int,
  deque < int >> accessTimes; // Access times for each page.
  int minSize = numeric_limits < int > ::max();
  int maxSize = 0;

  public: WorkingSetStrategy(int windowSize): windowSize(windowSize) {}

  void handlePageFault(Process & proc, int page) {
    currentTime++;
    proc.fault_count++;

    if (proc.page_table.find(page) != proc.page_table.end() && proc.page_table[page]) {} else {
      if (proc.page_stack.size() >= proc.total_pages) {
        int pageToEvict = findPageToEvict(proc);
        if (pageToEvict != -1) {
          evictPage(proc, pageToEvict);
        }
      }
      loadPage(proc, page);
    }
    updateAccessTime(page);
  }

  void updateAccessTime(int page) {
    while (!accessTimes[page].empty() && accessTimes[page].front() < currentTime - windowSize) {
      accessTimes[page].pop_front();
    }
    accessTimes[page].push_back(currentTime);

    // After updating the access time, update the min and max sizes.
    updateWorkingSetSize();
  }

  void updateWorkingSetSize() {
    int currentSize = 0;
    for (auto & entry: accessTimes) {
      if (!entry.second.empty()) {
        currentSize++;
      }
    }
    minSize = min(minSize, currentSize);
    maxSize = max(maxSize, currentSize);
  }

  // Getters for min and max sizes
  int getMinSize() const {
    return minSize;
  }

  int getMaxSize() const {
    return maxSize;
  }

  int findPageToEvict(Process & proc) {
    int oldestAccessTime = numeric_limits < int > ::max();
    int pageToEvict = -1;

    for (auto & page: proc.page_table) {
      if (page.second && !accessTimes[page.first].empty()) {
        int lastTime = accessTimes[page.first].front();
        if (lastTime < currentTime - windowSize && lastTime < oldestAccessTime) {
          oldestAccessTime = lastTime;
          pageToEvict = page.first;
        }
      }
    }

    return pageToEvict;
  }

  void evictPage(Process & proc, int page) {
    proc.page_table[page] = false;
    removePageFromStack(proc.page_stack, page);
  }

  void loadPage(Process & proc, int page) {
    proc.page_stack.push(page);
    proc.page_table[page] = true;
  }

  void removePageFromStack(stack < int > & pageStack, int pageToRemove) {
    stack < int > tempStack;
    while (!pageStack.empty()) {
      int top = pageStack.top();
      pageStack.pop();
      if (top != pageToRemove) {
        tempStack.push(top);
      }
    }
    while (!tempStack.empty()) {
      pageStack.push(tempStack.top());
      tempStack.pop();
    }
  }
};

class LRU_X_Strategy: public PageReplacementStrategy {
  private: int x; // The X in LRU-X
  unordered_map < int,
  list < int >> pageAccessTimes; // Stores the last X times a page was accessed.

  public: LRU_X_Strategy(int x): x(x) {}

  void handlePageFault(Process & proc, int page) override {
    currentTime++; // Increment the global time each time a fault handler is called.

    // Increment fault count whenever a page fault occurs.
    proc.fault_count++;

    // Check if page is in memory
    if (proc.page_table.find(page) != proc.page_table.end() && proc.page_table[page] == 1) {
      updateAccessTime(page);
    } else {
      if (proc.page_stack.size() >= proc.total_pages) {
        int pageToEvict = findPageToEvict();
        if (pageToEvict != -1) {
          evictPage(proc, pageToEvict);
        }
      }
      loadPage(proc, page);
    }
  }

  void updateAccessTime(int page) {
    if (pageAccessTimes[page].size() >= x) {
      pageAccessTimes[page].pop_front(); // Keep only the last X accesses
    }
    pageAccessTimes[page].push_back(currentTime);
  }
  int findPageToEvict() {
    int oldestTime = numeric_limits < int > ::max();
    int pageToEvict = -1;

    for (auto & entry: pageAccessTimes) {
      if (entry.second.size() == x) { // Only consider pages with exactly X accesses
        int time = entry.second.front(); // The oldest of the X accesses
        if (time < oldestTime) {
          oldestTime = time;
          pageToEvict = entry.first;
        }
      }
    }

    return pageToEvict;
  }

  void evictPage(Process & proc, int page) {
    // Remove the page from the page table and stack
    proc.page_table[page] = 0;
    // Here, we assume a simplistic stack-based removal for the demonstration
    stack < int > tempStack;
    while (!proc.page_stack.empty()) {
      int topPage = proc.page_stack.top();
      proc.page_stack.pop();
      if (topPage != page) {
        tempStack.push(topPage);
      }
    }

    while (!tempStack.empty()) {
      proc.page_stack.push(tempStack.top());
      tempStack.pop();
    }

  }

  void loadPage(Process & proc, int page) {
    proc.page_stack.push(page);
    proc.page_table[page] = 1;
    updateAccessTime(page);
  }

};

class LFUStrategy: public PageReplacementStrategy {
  private: unordered_map < int,
  int > frequency; // Keeps track of the access frequency of each page
  unordered_map < int,
  list < int > ::iterator > pageListMap; // Maps page numbers to their position in the list
  list < int > pageList; // Maintains the list of pages sorted by frequency and then by the most recent access

  public: void handlePageFault(Process & proc, int page) override {
    auto pageTableIt = proc.page_table.find(page);

    if (pageTableIt != proc.page_table.end() && pageTableIt -> second == 1) {
      // Page is already in memory, update frequency
      updateFrequency(page);
    } else {
      // New page fault, add page to memory
      if (proc.page_stack.size() >= proc.total_pages) {
        // Memory is full, need to remove the least frequently used page
        evictPage(proc);
      }
      loadPage(proc, page);
    }
  }

  void updateFrequency(int page) {
    // Increase the frequency count
    ++frequency[page];
    // Move the page to the correct position in the list based on its new frequency
    movePageInList(page);
  }

  void loadPage(Process & proc, int page) {
    // Load the new page into memory
    proc.page_stack.push(page);
    proc.page_table[page] = 1; // Mark this page as in memory
    frequency[page] = 1; // Set or reset frequency to 1

    // Add to the frequency list
    auto it = pageList.insert(pageList.end(), page);
    pageListMap[page] = it;

    proc.fault_count++;
  }

  void evictPage(Process & proc) {
    // Identify the least frequently used page, which will be at the front of the list
    int lfuPage = pageList.front();
    pageList.pop_front();
    pageListMap.erase(lfuPage);

    // Remove from the page table and stack
    proc.page_table[lfuPage] = 0;
    proc.page_stack.pop(); // Assuming a compatible stack operation
    frequency.erase(lfuPage);

  }

  void movePageInList(int page) {
    // Remove the page from its current position
    pageList.erase(pageListMap[page]);
    // Re-insert the page into the list at the position sorted by frequency and most recent use
    auto it = pageList.end();
    do {
      --it;
      if (frequency[ * it] <= frequency[page]) {
        it = pageList.insert(++it, page);
        pageListMap[page] = it;
        return;
      }
    } while (it != pageList.begin());

    // If the page has the lowest frequency or list is empty
    it = pageList.insert(pageList.begin(), page);
    pageListMap[page] = it;
  }

};

class OPTLookaheadStrategy: public PageReplacementStrategy {
  public: int lookahead;

  OPTLookaheadStrategy(int lookahead): lookahead(lookahead) {}

  void handlePageFault(Process & proc, int page) override {

    // Check if the page is already in the page table, initialize if not
    if (proc.page_table.count(page) == 0) {
      proc.page_table[page] = 0; // Initialize page status as not in memory
    }

    // If page is not in memory, a page fault occurs
    if (proc.page_table[page] == 0) {
      proc.fault_count++; // Increment the fault count for this process

      // Check if the memory (represented by the stack) is full
      if (proc.page_stack.size() >= proc.total_pages) {
        int evicted_page = choosePageToEvict(proc); // Choose a page to evict based on the lookahead strategy
        if (evicted_page != -1) {
          // Remove the evicted page from the stack
          stack < int > tempStack;
          while (!proc.page_stack.empty() && proc.page_stack.top() != evicted_page) {
            tempStack.push(proc.page_stack.top());
            proc.page_stack.pop();
          }
          // Remove the evicted page
          if (!proc.page_stack.empty()) {
            proc.page_stack.pop();
          }
          // Put back the other pages
          while (!tempStack.empty()) {
            proc.page_stack.push(tempStack.top());
            tempStack.pop();
          }
          proc.page_table[evicted_page] = 0; // Mark the evicted page as not in memory
        }
      }
      // Load the new page into the stack and update page table
      proc.page_stack.push(page);
      proc.page_table[page] = 1; // Mark this page as in memory
    }
  }

  int choosePageToEvict(const Process & proc) {
    unordered_map < int, int > futureAccesses;
    int farthestAccess = -1;
    int pageToEvict = -1;

    // Create a copy of the stack to work with
    stack < int > copyStack = proc.page_stack;
    vector < int > tempStack;

    // Transfer elements from the copy to a temporary vector for easier manipulation
    while (!copyStack.empty()) {
      tempStack.push_back(copyStack.top());
      copyStack.pop();
    }
    reverse(tempStack.begin(), tempStack.end());

    // Assume current position is at the last page access that caused a fault
    int currentPosition = find(proc.page_access_sequence.begin(), proc.page_access_sequence.end(), tempStack.back()) - proc.page_access_sequence.begin();

    // Calculate the next access time for each page in memory within the lookahead window
    int windowEnd = min(currentPosition + this -> lookahead, int(proc.page_access_sequence.size()));
    for (int i = currentPosition + 1; i < windowEnd; ++i) {
      int currentPage = proc.page_access_sequence[i];
      // Only consider pages currently in memory and not already assigned a future access
      if (proc.page_table.count(currentPage) > 0 && proc.page_table.at(currentPage) == 1 && futureAccesses.find(currentPage) == futureAccesses.end()) {
        futureAccesses[currentPage] = i;
      }
    }

    // Determine which page to evict based on the latest access in the lookahead window
    for (const int page: tempStack) {
      auto found = futureAccesses.find(page);
      if (found == futureAccesses.end()) {
        // If no future access is found within the window, immediately select this page for eviction
        return page;
      } else if (found -> second > farthestAccess) {
        // Find the page with the farthest future access
        farthestAccess = found -> second;
        pageToEvict = page;
      }
    }

    return pageToEvict;
  }

};

class MRUStrategy: public PageReplacementStrategy {
  private: list < int > mruList; // Stores the most recently used pages with the MRU page at the front
  unordered_map < int,
  list < int > ::iterator > pageMap; // Maps page numbers to their position in the MRU list

  public: void handlePageFault(Process & proc, int page) override {

    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
      // Page is already in memory, move it to the front (MRU position)
      mruList.erase(it -> second);
      mruList.push_front(page);
      pageMap[page] = mruList.begin();
    } else {
      // New page fault, add to MRU position
      if (proc.page_stack.size() >= proc.total_pages) {
        // Memory is full, need to remove the least recently used page (back of the list)
        int leastUsedPage = mruList.back();
        mruList.pop_back();
        pageMap.erase(leastUsedPage);
        proc.page_table[leastUsedPage] = 0; // Mark this page as not in memory
      }

      // Load the new page into memory
      mruList.push_front(page);
      pageMap[page] = mruList.begin();
      proc.page_table[page] = 1; // Mark this page as in memory
      proc.page_stack.push(page); // Simulating stack operation for compatibility
      proc.fault_count++;
    }

  }
};

class LIFOStrategy: public PageReplacementStrategy {
  public: void handlePageFault(Process & proc, int page) override {
    // Ensure the page is in the page table, initialize if not present
    if (proc.page_table.count(page) == 0) {
      proc.page_table[page] = 0; // Page not in memory, initialized lazily
    }

    // Page not in memory
    if (proc.page_table[page] == 0) {
      proc.fault_count++;

      // Check if memory (represented by the stack) is full
      if (proc.page_stack.size() >= proc.total_pages) {
        int evicted_page = proc.page_stack.top();
        proc.page_stack.pop();
        proc.page_table[evicted_page] = 0; // Mark this page as not in memory
      }

      // Load the new page
      proc.page_stack.push(page);
      proc.page_table[page] = 1; // Mark this page as in memory
    }
  }
};

struct SimulationConfig {
  int total_page_frames;
  int page_size;
  int frames_per_process_or_delta;
  int lookahead_or_window_size;
  int min_free_pool_size;
  int max_free_pool_size;
  int num_processes;
  vector < Process > processes;
  unique_ptr < PageReplacementStrategy > strategy;
};

bool parseInput(istream & input, SimulationConfig & config) {
  cout << "Parsing configuration..." << endl;
  if (!(input >> config.total_page_frames >> config.page_size >> config.frames_per_process_or_delta >>
      config.lookahead_or_window_size >> config.min_free_pool_size >> config.max_free_pool_size >>
      config.num_processes)) {
    cerr << "Failed to parse global settings" << endl;
    return false;
  }

  input.ignore(numeric_limits < streamsize > ::max(), '\n'); // Clear the newline

  for (int i = 0; i < config.num_processes; ++i) {
    Process proc;
    string line;
    getline(input, line);
    istringstream iss(line);
    if (!(iss >> proc.process_id >> proc.total_pages)) {
      cerr << "Error parsing process details for process " << i + 1 << endl;
      return false;
    }

    int page;
    while (iss >> page) {
      proc.page_access_sequence.push_back(page);
    }

    config.processes.push_back(proc);
  }

  return true;
}

void simulatePageFaults(Process & proc, PageReplacementStrategy & strategy) {
  for (int page: proc.page_access_sequence) {
    strategy.handlePageFault(proc, page);
  }
}

int runSimulation(SimulationConfig & config) {
  int total_replacements = 0;
  for (auto & proc: config.processes) {
    simulatePageFaults(proc, * config.strategy);
    total_replacements += proc.fault_count;
  }
  return total_replacements;
}

unique_ptr < PageReplacementStrategy > createStrategy(const string & type, int x) {
  if (type == "LIFO") {
    return make_unique < LIFOStrategy > ();
  } else if (type == "MRU") {
    return make_unique < MRUStrategy > ();
  } else if (type == "OPT-Lookahead-X") {
    return make_unique < OPTLookaheadStrategy > (x);
  } else if (type == "LFU") {
    return make_unique < LFUStrategy > ();
  } else if (type == "LRU-X") {
    return make_unique < LRU_X_Strategy > (x);
  } else if (type == "WS") {
    return make_unique < WorkingSetStrategy > (x);
  } else {
    throw runtime_error("Unsupported strategy type provided: " + type);
  }
}

int main() {
  ifstream file("input.txt");
  if (!file) {
    cerr << "Failed to open input file." << endl;
    return 1;
  }

  // Parsing configuration once
  SimulationConfig config;
  if (!parseInput(file, config)) {
    cerr << "Failed to parse input correctly." << endl;
    return 1;
  }

  vector < pair < string, int >> strategies = {
    {
      "LIFO",
      0
    },
    {
      "MRU",
      0
    },
    {
      "OPT-Lookahead-X",
      config.lookahead_or_window_size
    }, // Dynamic lookahead
    {
      "LFU",
      0
    },
    {
      "LRU-X",
      config.lookahead_or_window_size
    },
    {
      "WS",
      config.lookahead_or_window_size
    }
  };

  for (const auto & strategyInfo: strategies) {
    file.clear(); // Clear any error flags
    file.seekg(0); // Reset file read position to the beginning for each strategy
    config.strategy = createStrategy(strategyInfo.first, strategyInfo.second);

    // Resetting fault count before each simulation
    for (auto & proc: config.processes) {
      proc.fault_count = 0;
      proc.page_stack = stack < int > (); // Clearing the page stack
      proc.page_table.clear(); // Clearing the page table
    }

    int total_replacements = runSimulation(config);

    cout << "Using " << strategyInfo.first << " Algorithm:\n";
    cout << "Total Faults\n";
    for (const auto & proc: config.processes) {
      cout << "Current Page Fault For " << proc.process_id << ": " << proc.fault_count << endl;
    }
    cout << "-- Total Replacements: " << total_replacements << " --\n\n";

    if (strategyInfo.first == "WS") {
      auto wsStrategy = dynamic_cast < WorkingSetStrategy * > (config.strategy.get());
      if (wsStrategy) {
        cout << "Min Working Set Size: " << wsStrategy -> getMinSize() << endl;
        cout << "Max Working Set Size: " << wsStrategy -> getMaxSize() << endl;
      }
    }

  }

  return 0;
}