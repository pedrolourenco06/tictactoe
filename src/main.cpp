#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdlib>
#include <ctime>

class TicTacToe {
public:
    TicTacToe() : board(3, std::vector<char>(3, ' ')), currentPlayer('X'), gameEnded(false) {}

    void displayBoard() {
        std::cout << "Current Board:\n";
        for (const auto& row : board) {
            for (const auto& cell : row) {
                std::cout << (cell == ' ' ? '.' : cell) << " ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    }

    bool makeMove(char player, int row, int col) {
        std::lock_guard<std::mutex> lock(boardMutex);
        if (row < 0 || row >= 3 || col < 0 || col >= 3) {
            std::cout << "Invalid move position: (" << row << ", " << col << ")\n";
            return false;
        }
        if (board[row][col] != ' ') {
            std::cout << "Position (" << row << ", " << col << ") already occupied.\n";
            return false;
        }
        
        board[row][col] = player;
        displayBoard();
        
        if (checkWin(player)) {
            std::cout << "Player " << player << " wins!\n";
            gameEnded = true;
            turnCv.notify_all();
            return true;
        }

        // Verifica se o tabuleiro está cheio após cada jogada
        if (isBoardFull()) {
            std::cout << "Game ended in a draw.\n";
            gameEnded = true;
            turnCv.notify_all();
        }

        togglePlayer(); // Alterna o jogador após uma jogada válida
        return false;
    }

    bool checkWin(char player) {
        // Check rows, columns, and diagonals
        for (int i = 0; i < 3; ++i) {
            if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
                (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
                return true;
            }
        }
        if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
            (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
            return true;
        }
        return false;
    }

    bool isBoardFull() {
        for (const auto& row : board) {
            for (const auto& cell : row) {
                if (cell == ' ') {
                    return false;
                }
            }
        }
        return true;
    }

    bool isGameOver() {
        std::lock_guard<std::mutex> lock(boardMutex);
        return gameEnded;
    }

    void togglePlayer() {
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
        turnCv.notify_all(); // Notifica para que o próximo jogador saiba que é a sua vez
    }

    char getCurrentPlayer() const { return currentPlayer; }

    void waitForTurn(char player) {
        std::unique_lock<std::mutex> lock(turnMutex);
        turnCv.wait(lock, [&] { return currentPlayer == player || gameEnded; });
    }

private:
    std::vector<std::vector<char>> board;
    char currentPlayer;
    bool gameEnded;
    mutable std::mutex boardMutex;
    std::mutex turnMutex;
    std::condition_variable turnCv;
};

class Player {
public:
    Player(TicTacToe& game, char symbol) : game(game), symbol(symbol) {}

    void playSequential() {
        for (int i = 0; i < 3 && !game.isGameOver(); ++i) {
            for (int j = 0; j < 3 && !game.isGameOver(); ++j) {
                game.waitForTurn(symbol);
                if (game.isGameOver()) return;

                std::cout << "Player " << symbol << " attempting move at " << i << ", " << j << std::endl;
                if (game.makeMove(symbol, i, j)) return;
            }
        }
    }

    void playRandom() {
        std::srand(std::time(nullptr));
        while (!game.isGameOver()) {
            game.waitForTurn(symbol);
            if (game.isGameOver()) return;

            int row = std::rand() % 3;
            int col = std::rand() % 3;

            std::cout << "Player " << symbol << " attempting random move at " << row << ", " << col << std::endl;
            if (game.makeMove(symbol, row, col)) return;
        }
    }

private:
    TicTacToe& game;
    char symbol;
};

int main() {
    TicTacToe game;
    Player playerX(game, 'X');
    Player playerO(game, 'O');

    std::thread t1(&Player::playSequential, &playerX);
    std::thread t2(&Player::playRandom, &playerO);

    t1.join();
    t2.join();

    if (!game.isGameOver()) {
        std::cout << "Game ended in a draw.\n";
    }

    return 0;
}
