CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET   = ackit
SRC      = main.cpp
PREFIX   = /usr/local

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)
	@echo "インストール完了: $(PREFIX)/bin/$(TARGET)"
	@echo "テンプレートを作成してください:"
	@echo "  mkdir -p ~/.config/ackit && vim ~/.config/ackit/template.cpp"

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	@echo "アンインストール完了"

clean:
	rm -f $(TARGET)