CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
INCLUDE = -Iinclude

SRC = src/main.cpp src/Inventory.cpp src/Cart.cpp \
      src/RecipeEngine.cpp src/Analytics.cpp src/FileManager.cpp \
      src/StoreLocator.cpp src/ShoppingAssistant.cpp \
      src/Management.cpp src/CsvImporter.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = smartmart

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET)