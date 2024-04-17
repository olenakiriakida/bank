#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstdlib> 
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <stdexcept> 
using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

SOCKET ConnectSocket = INVALID_SOCKET;

void HideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1; 
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

DWORD WINAPI Sender(LPVOID param) {
    while (true) {

    }
    return 0;
}

DWORD WINAPI Receiver(LPVOID param) {
    while (true) {
        char answer[DEFAULT_BUFLEN];
        int iResult = recv(ConnectSocket, answer, DEFAULT_BUFLEN, 0);
        answer[iResult] = '\0';

        if (iResult > 0) {
            cout << answer << "\n";
        }
        else if (iResult == 0)
            cout << "соединение с сервером закрыто.\n";
        else
            cout << "recv завершился с ошибкой: " << WSAGetLastError() << "\n";
    }
    return 0;
}

class AccountHolder {
public:

    void setLastName(const string& lastName) { this->lastName = lastName; }
    void setFirstName(const string& firstName) { this->firstName = firstName; }
    void setBalance(double balance) { this->balance = balance; }
    void setCreditRating(int credit_ring) { this->credit_ring = credit_ring; }
    void setRegistrationDate(string date) { this->date = date; }

    const string& getLastName() const { return lastName; }
    const string& getFirstName() const { return firstName; }
    double getBalance() const { return balance; }
    double setCreditRating() const { return credit_ring; }
    const string setRegistrationDate() const { return date; }

private:
    string lastName;
    string firstName;
    double balance;
    string date;
    double credit_ring;
};

class Operation {
public:
    enum Type {
        INCOME,
        EXPENSE
    };

    Type operationType;
    time_t creationDate;
    bool status;

    Operation(Type type) : operationType(type), creationDate(time(0)), status(true) {}
};

class Account {
public:

    void setHolder(const AccountHolder& holder) { this->holder = holder; }
    void setAccountNumber(int accountNumber) { this->accountNumber = accountNumber; }


    AccountHolder& getHolder() { return holder; }
    int getAccountNumber() const { return accountNumber; }

    void TransferMoney(Account& destinationAccount, double amount) {

        if (holder.getBalance() >= amount) {

            this_thread::sleep_for(chrono::seconds(2));


            mutex.lock();

            holder.setBalance(holder.getBalance() - amount);

            double dest = destinationAccount.getHolder().getBalance();
            destinationAccount.getHolder().setBalance(dest + amount);

            mutex.unlock();

            cout << "Перевод завершен успешно. Баланс счета " << getAccountNumber() << ": " << holder.getBalance()
                << ", Баланс счета " << destinationAccount.getAccountNumber() << ": "
                << destinationAccount.getHolder().getBalance() << endl;
        }
        else {
            cout << "Недостаточно средств для перевода." << endl;
            throw runtime_error("Недостаточно средств для перевода.");
        }
    }

private:
    AccountHolder holder;
    int accountNumber;
    static mutex mutex;

    int generateAccountNumber() {
        static int nextAccountNumber = 1;
        return nextAccountNumber++;
    }
};

mutex Account::mutex;

void ShowAllAccounts(vector<Account>& accounts) {
    cout << "\nСписок всех счетов и их балансов:\n"; 
    for (size_t i = 0; i < accounts.size(); ++i) {
        Account& account = accounts[i];
        cout << i + 1 << ". Карта " << account.getHolder().getLastName() << ": " << account.getHolder().getBalance() << " USD\n";
    }
}



class BankAccount {
public:
    AccountHolder owner;
    double balance;

    BankAccount(const AccountHolder& owner, double initialBalance = 0.0) : owner(owner), balance(initialBalance) {}
};

void TransferMoneyInBackground(double amount) {

    cout << "Перевод " << amount << " выполняется в фоновом режиме...\n"; 

    this_thread::sleep_for(chrono::seconds(5)); 
    cout << "Перевод выполнен.\n";
}
void CancelPayment() {

    cout << "Платеж отменен.\n";
}

void TransferThread(Account& senderAccount, Account& receiverAccount, double paymentAmount) {
    try {
        senderAccount.TransferMoney(receiverAccount, paymentAmount);
        cout << "Перевод выполнен.\n";
    }
    catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << "\n";
        CancelPayment();
    }
    this_thread::sleep_for(chrono::seconds(1)); 
}

int main() {
    setlocale(0, "");
    system("title BANK SYSTEM");

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup завершилась с ошибкой: " << iResult << "\n";
        return 11;
    }

    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const char* ip = "localhost";
    addrinfo* result = NULL;
    iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);

    if (iResult != 0) {
        cout << "getaddrinfo завершилась с ошибкой: " << iResult << "\n";
        WSACleanup();
        return 12;
    }

    for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) {
            cout << "socket завершилась с ошибкой: " << WSAGetLastError() << "\n";
            WSACleanup();
            return 13;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        cout << "невозможно подключиться к серверу!\n";
        WSACleanup();
        return 14;
    }


    HANDLE senderThreadHandle = CreateThread(0, 0, Sender, 0, 0, 0);
    HANDLE receiverThreadHandle = CreateThread(0, 0, Receiver, 0, 0, 0);

    if (senderThreadHandle == NULL || receiverThreadHandle == NULL) {
        cout << "Ошибка при создании потоков.\n";
        closesocket(ConnectSocket);
        WSACleanup();
        return 16;
    }

    vector<Account> accounts;

   
    AccountHolder ah1;
    ah1.setFirstName("Syd");
    ah1.setLastName("Barrett");
    ah1.setBalance(10000);
    ah1.setCreditRating(4.3);
    ah1.setRegistrationDate("01.01.2010");
    Account a1;
    a1.setAccountNumber(1001);
    a1.setHolder(ah1);
    accounts.push_back(a1);
  
    AccountHolder ah2;
    ah2.setFirstName("Nick");
    ah2.setLastName("Mason");
    ah2.setBalance(15000);
    ah2.setCreditRating(3.8);
    ah2.setRegistrationDate("15.05.2015");
    Account a2;
    a2.setAccountNumber(1002);
    a2.setHolder(ah2);
    accounts.push_back(a2);

    AccountHolder ah3;
    ah3.setFirstName("Rick");
    ah3.setLastName("Wright");
    ah3.setBalance(20000);
    ah3.setCreditRating(4.5);
    ah3.setRegistrationDate("20.12.2018");
    Account a3;
    a3.setAccountNumber(1003);
    a3.setHolder(ah3);
    accounts.push_back(a3);

    ShowAllAccounts(accounts);

    closesocket(ConnectSocket);
    WSACleanup();

    cout << "Программа успешно завершена.\n";

    return 0;
}