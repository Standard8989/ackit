/**
 * ackit - AtCoder ABC helper tool
 *
 * Usage:
 *   ackit make abc300        ... ABC300/a.cpp ~ e.cpp をテンプレから作成
 *   ackit test a             ... ABC???/ 内で実行、A問題をテスト
 *   ackit remove abc300      ... ABC300/ ディレクトリを削除
 *
 * Build:
 *   g++ -std=c++17 -O2 -o ackit ackit.cpp
 *   sudo mv ackit /usr/local/bin/
 *
 * Requirements:
 *   - oj (online-judge-tools) がインストール済みであること
 *   - ~/.config/ackit/template.cpp がテンプレートとして存在すること
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define ACKIT_VERSION "1.0.0"

namespace fs = std::filesystem;

// ----------------------------------------------------------------
// 設定
// ----------------------------------------------------------------
const fs::path TEMPLATE_PATH =
    fs::path(std::getenv("HOME")) / ".config/ackit/template.cpp";

const std::vector<std::string> PROBLEMS = {"a", "b", "c", "d", "e"};

// ----------------------------------------------------------------
// ユーティリティ
// ----------------------------------------------------------------
static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

static bool run(const std::string& cmd) {
    int ret = std::system(cmd.c_str());
    return ret == 0;
}

// ----------------------------------------------------------------
// ackit version
// ----------------------------------------------------------------
static int cmd_version() {
    std::cout << "ackit version " << ACKIT_VERSION << "\n";
    return 0;
}

// ----------------------------------------------------------------
// ackit make <contest>
// ----------------------------------------------------------------
static int cmd_make(const std::string& contest_raw) {
    const std::string contest_lower = to_lower(contest_raw);
    const std::string contest_upper = to_upper(contest_raw);

    if (contest_lower.size() < 4 || contest_lower.substr(0, 3) != "abc") {
        std::cerr << "[ackit] エラー: コンテスト名は abc??? の形式で指定してください\n";
        return 1;
    }

    if (!fs::exists(TEMPLATE_PATH)) {
        std::cerr << "[ackit] エラー: テンプレートが見つかりません: " << TEMPLATE_PATH << "\n";
        std::cerr << "           mkdir -p ~/.config/ackit && touch ~/.config/ackit/template.cpp\n";
        return 1;
    }

    std::ifstream tmpl(TEMPLATE_PATH);
    if (!tmpl) {
        std::cerr << "[ackit] エラー: テンプレートを開けません\n";
        return 1;
    }
    std::string tmpl_content((std::istreambuf_iterator<char>(tmpl)),
                              std::istreambuf_iterator<char>());

    fs::path contest_dir = fs::current_path() / contest_upper;
    if (fs::exists(contest_dir)) {
        std::cerr << "[ackit] 警告: " << contest_upper << " はすでに存在します\n";
    } else {
        fs::create_directory(contest_dir);
        std::cout << "[ackit] ディレクトリ作成: " << contest_upper << "/\n";
    }

    for (const auto& prob : PROBLEMS) {
        fs::path cpp_path = contest_dir / (prob + ".cpp");
        if (fs::exists(cpp_path)) {
            std::cout << "  [skip] " << prob << ".cpp (すでに存在)\n";
            continue;
        }
        std::ofstream ofs(cpp_path);
        if (!ofs) {
            std::cerr << "[ackit] エラー: " << cpp_path << " を作成できません\n";
            return 1;
        }
        ofs << tmpl_content;
        std::cout << "  [作成] " << prob << ".cpp\n";
    }

    std::cout << "[ackit] 完了: " << contest_upper << "/ を作成しました\n";
    return 0;
}

// ----------------------------------------------------------------
// ackit test <problem>
// ----------------------------------------------------------------
static int cmd_test(const std::string& prob_raw) {
    const std::string prob = to_lower(prob_raw);

    fs::path cwd = fs::current_path();
    std::string dir_name  = cwd.filename().string();
    std::string dir_lower = to_lower(dir_name);

    if (dir_lower.size() < 4 || dir_lower.substr(0, 3) != "abc") {
        std::cerr << "[ackit] エラー: ABC??? ディレクトリ内で実行してください\n";
        return 1;
    }

    std::string task_name = dir_lower + "_" + prob;
    std::string url = "https://atcoder.jp/contests/" + dir_lower +
                      "/tasks/" + task_name;

    fs::path src = cwd / (prob + ".cpp");
    if (!fs::exists(src)) {
        std::cerr << "[ackit] エラー: " << prob << ".cpp が見つかりません\n";
        return 1;
    }

    // テストケース用ディレクトリ
    fs::path tests_dir = cwd / ("test_" + prob);
    if (!fs::exists(tests_dir) || fs::is_empty(tests_dir)) {
        std::cout << "[ackit] テストケースをダウンロード中: " << url << "\n";
        std::string dl_cmd = "oj download --directory " +
                             tests_dir.string() + " " + url;
        if (!run(dl_cmd)) {
            std::cerr << "[ackit] エラー: oj download に失敗しました\n";
            std::cerr << "           AtCoder にログイン済みか確認: oj login https://atcoder.jp\n";
            return 1;
        }
    } else {
        std::cout << "[ackit] キャッシュ済みのテストケースを使用: " << tests_dir << "\n";
    }

    // output/ error/ ディレクトリ
    fs::path out_dir = cwd / "output";
    fs::path err_dir = cwd / "error";
    if (!fs::exists(out_dir)) fs::create_directory(out_dir);
    if (!fs::exists(err_dir)) fs::create_directory(err_dir);

    fs::path bin     = out_dir / (prob + ".out");
    fs::path err_log = err_dir / (prob + ".txt");

    // コンパイル（stderr → error/?.txt）
    std::string compile_cmd = "g++ -std=c++17 -O2 -o " + bin.string() +
                              " " + src.string() +
                              " 2>" + err_log.string();
    std::cout << "[ackit] コンパイル中: g++ -std=c++17 -O2 -o "
              << bin.string() << " " << src.string() << "\n";
    if (!run(compile_cmd)) {
        std::cerr << "[ackit] エラー: コンパイル失敗（詳細: " << err_log.string() << "）\n";
        run("cat " + err_log.string());
        return 1;
    }

    // テストケースごとに手動で実行し、stderr をケース名付きで error/?.txt に記録
    // oj test はケース名を環境変数で渡さないため、自前でループする
    std::cout << "[ackit] テスト実行中... (stderr → " << err_log.string() << ")\n";

    // err_log をリセット（前回の内容を消す）
    { std::ofstream ofs(err_log, std::ios::trunc); }

    // テストケース一覧を列挙（*.in ファイル）
    std::vector<fs::path> in_files;
    for (const auto& entry : fs::directory_iterator(tests_dir)) {
        if (entry.path().extension() == ".in") {
            in_files.push_back(entry.path());
        }
    }
    std::sort(in_files.begin(), in_files.end());

    // 一時ラッパーを生成（入力ファイルを受け取り stderr をリダイレクト）
    fs::path wrapper = fs::temp_directory_path() / ("ackit_" + dir_lower + "_" + prob + ".sh");
    {
        std::ofstream sh(wrapper);
        sh << "#!/bin/sh\n";
        sh << bin.string() << " \"$@\" 2>>" << err_log.string() << "\n";
    }
    run("chmod +x " + wrapper.string());

    // ケースごとに stderr ヘッダを挟んでから oj test を1ケースずつ実行
    for (const auto& in_file : in_files) {
        std::string case_name = in_file.stem().string();  // "sample-1"
        // ヘッダを err_log に追記
        {
            std::ofstream ofs(err_log, std::ios::app);
            ofs << "\n=== " << case_name << " ===\n";
        }
        std::string single_test_cmd = "oj test --directory " + tests_dir.string() +
                                      " --command " + wrapper.string() +
                                      " " + in_file.string();
        run(single_test_cmd);
    }

    return 0;
}

// ----------------------------------------------------------------
// ackit remove <contest>
// ----------------------------------------------------------------
static int cmd_remove(const std::string& contest_raw) {
    const std::string contest_lower = to_lower(contest_raw);
    const std::string contest_upper = to_upper(contest_raw);

    if (contest_lower.size() < 4 || contest_lower.substr(0, 3) != "abc") {
        std::cerr << "[ackit] エラー: コンテスト名は abc??? の形式で指定してください\n";
        return 1;
    }

    fs::path contest_dir = fs::current_path() / contest_upper;
    if (!fs::exists(contest_dir)) {
        std::cerr << "[ackit] エラー: " << contest_upper << " が見つかりません\n";
        return 1;
    }

    std::cout << "[ackit] " << contest_upper << "/ を削除します。よろしいですか？ [y/N]: ";
    std::string answer;
    std::getline(std::cin, answer);
    if (answer != "y" && answer != "Y") {
        std::cout << "[ackit] キャンセルしました\n";
        return 0;
    }

    fs::remove_all(contest_dir);
    std::cout << "[ackit] 削除完了: " << contest_upper << "/\n";
    return 0;
}

// ----------------------------------------------------------------
// main
// ----------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "使い方:\n"
                  << "  ackit make <contest>     例: ackit make abc300\n"
                  << "  ackit test <problem>     例: (ABC300/ 内で) ackit test a\n"
                  << "  ackit remove <contest>   例: ackit remove abc300\n";
        return 0;
    }

    std::string subcmd = to_lower(argv[1]);

    if (subcmd == "version") {
        return cmd_version();
    } else if (subcmd == "make" || subcmd == "remove") {
        if (argc < 3) {
            std::cerr << "[ackit] エラー: コンテスト名を指定してください\n";
            return 1;
        }
        if (subcmd == "make")   return cmd_make(argv[2]);
        if (subcmd == "remove") return cmd_remove(argv[2]);
    } else if (subcmd == "test") {
        if (argc < 3) {
            std::cerr << "[ackit] エラー: 問題名を指定してください (例: a)\n";
            return 1;
        }
        return cmd_test(argv[2]);
    } else {
        std::cerr << "[ackit] エラー: 不明なサブコマンド: " << subcmd << "\n";
        std::cerr << "           make / test / remove のどちらかを指定してください\n";
        return 1;
    }
}