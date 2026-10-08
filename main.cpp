/**
 * ackit - AtCoder ABC helper tool
 *
 * Usage:
 *   ackit make abc300        ... ABC300/a.cpp ~ e.cpp をテンプレから作成
 *   ackit test a             ... ABC???/ 内で実行、A問題をテスト
 *   ackit remove abc300      ... ABC300/ ディレクトリを削除
 *   ackit fill d abc300 abc400  ... dfill/ にabc300~abc400のd.cppを集めて管理
 *   ackit fill test d abc300    ... dfill/ 内でabc300のd問題をテスト
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

#define ACKIT_VERSION "1.3.0"

namespace fs = std::filesystem;

// ----------------------------------------------------------------
// 設定
// ----------------------------------------------------------------
const fs::path TEMPLATE_PATH =
    fs::path(std::getenv("HOME")) / ".config/ackit/template.cpp";

const std::vector<std::string> PROBLEMS = {"a", "b", "c", "d", "e", "f"};

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
// テスト共通ロジック
//   src_file : ソースファイルパス
//   contest  : "abc300" などの小文字コンテスト名
//   prob     : "a" などの小文字問題名
//   work_dir : output/ error/ test_?/ を置くディレクトリ
// ----------------------------------------------------------------
static int run_test(const fs::path& src_file,
                    const std::string& contest_lower,
                    const std::string& prob,
                    const fs::path& work_dir) {
    std::string task_name = contest_lower + "_" + prob;
    std::string url = "https://atcoder.jp/contests/" + contest_lower +
                      "/tasks/" + task_name;

    // テストケース用ディレクトリ
    fs::path tests_dir = work_dir / ("test_" + contest_lower + "_" + prob);
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
    fs::path out_dir = work_dir / "output";
    fs::path err_dir = work_dir / "error";
    if (!fs::exists(out_dir)) fs::create_directory(out_dir);
    if (!fs::exists(err_dir)) fs::create_directory(err_dir);

    fs::path bin     = out_dir / (contest_lower + "_" + prob + ".out");
    fs::path err_log = err_dir / (contest_lower + "_" + prob + ".txt");

    // コンパイル（stderr → error/???.txt）
    std::string compile_cmd = "g++ -std=c++17 -O2 -o " + bin.string() +
                              " " + src_file.string() +
                              " 2>" + err_log.string();
    std::cout << "[ackit] コンパイル中: g++ -std=c++17 -O2 -o "
              << bin.string() << " " << src_file.string() << "\n";
    if (!run(compile_cmd)) {
        std::cerr << "[ackit] エラー: コンパイル失敗（詳細: " << err_log.string() << "）\n";
        run("cat " + err_log.string());
        return 1;
    }

    // テストケース一覧を列挙（*.in ファイル）
    std::cout << "[ackit] テスト実行中... (stderr → " << err_log.string() << ")\n";
    { std::ofstream ofs(err_log, std::ios::trunc); }

    std::vector<fs::path> in_files;
    for (const auto& entry : fs::directory_iterator(tests_dir)) {
        if (entry.path().extension() == ".in") {
            in_files.push_back(entry.path());
        }
    }
    std::sort(in_files.begin(), in_files.end());

    int ac_count = 0, wa_count = 0;

    auto trim = [](std::string s) {
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
            s.pop_back();
        return s;
    };

    for (const auto& in_file : in_files) {
        std::string case_name = in_file.stem().string();
        fs::path out_file = in_file;
        out_file.replace_extension(".out");

        std::ifstream ifs_expected(out_file);
        std::string expected((std::istreambuf_iterator<char>(ifs_expected)),
                              std::istreambuf_iterator<char>());

        fs::path tmp_out = fs::temp_directory_path() /
                           ("ackit_out_" + contest_lower + "_" + prob + "_" + case_name + ".txt");

        {
            std::ofstream ofs(err_log, std::ios::app);
            ofs << "\n=== " << case_name << " ===\n";
        }
        std::string exec_cmd = bin.string() +
                               " <" + in_file.string() +
                               " >" + tmp_out.string() +
                               " 2>>" + err_log.string();
        run(exec_cmd);

        std::ifstream ifs_actual(tmp_out);
        std::string actual((std::istreambuf_iterator<char>(ifs_actual)),
                            std::istreambuf_iterator<char>());

        bool ac = (trim(actual) == trim(expected));
        if (ac) {
            std::cout << "\033[32m[AC]\033[0m " << case_name << "\n";
            ++ac_count;
        } else {
            std::cout << "\033[31m[WA]\033[0m " << case_name << "\n";
            std::cout << "  expected: " << trim(expected) << "\n";
            std::cout << "  actual  : " << trim(actual)   << "\n";
            ++wa_count;
        }
        fs::remove(tmp_out);
    }

    std::cout << "\n結果: " << ac_count << " AC / " << wa_count << " WA\n";
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

    fs::path src = cwd / (prob + ".cpp");
    if (!fs::exists(src)) {
        std::cerr << "[ackit] エラー: " << prob << ".cpp が見つかりません\n";
        return 1;
    }

    return run_test(src, dir_lower, prob, cwd);
}

// ----------------------------------------------------------------
// ackit fill make <prob> <from> <to>
//   例: ackit fill make d abc300 abc400
// ackit fill test <prob> <contest>
//   例: ackit fill test d abc300
// ----------------------------------------------------------------
static int cmd_fill(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "[ackit] 使い方:\n"
                  << "  ackit fill make <prob> <from> <to>   例: ackit fill make d abc300 abc400\n"
                  << "  ackit fill test <prob> <contest>      例: ackit fill test d abc300\n";
        return 1;
    }

    std::string subcmd = to_lower(argv[0]);
    std::string prob   = to_lower(argv[1]);

    if (prob.size() != 1) {
        std::cerr << "[ackit] エラー: 問題名は1文字で指定してください (例: d)\n";
        return 1;
    }

    // fill ディレクトリ名: "dfill"
    std::string fill_dir_name = prob + "fill";
    fs::path fill_dir = fs::current_path() / fill_dir_name;

    if (subcmd == "make") {
        if (argc < 4) {
            std::cerr << "[ackit] エラー: ackit fill make <prob> <from> <to>\n";
            return 1;
        }
        std::string from_lower = to_lower(argv[2]);
        std::string to_lower_s = to_lower(argv[3]);

        // abc??? から番号を取り出す
        auto parse_num = [](const std::string& s) -> int {
            if (s.size() < 4 || s.substr(0, 3) != "abc") return -1;
            try { return std::stoi(s.substr(3)); } catch (...) { return -1; }
        };
        int from_num = parse_num(from_lower);
        int to_num   = parse_num(to_lower_s);
        if (from_num < 0 || to_num < 0 || from_num > to_num) {
            std::cerr << "[ackit] エラー: コンテスト番号が不正です\n";
            return 1;
        }

        // テンプレート読み込み
        if (!fs::exists(TEMPLATE_PATH)) {
            std::cerr << "[ackit] エラー: テンプレートが見つかりません: " << TEMPLATE_PATH << "\n";
            return 1;
        }
        std::ifstream tmpl(TEMPLATE_PATH);
        std::string tmpl_content((std::istreambuf_iterator<char>(tmpl)),
                                  std::istreambuf_iterator<char>());

        // fill ディレクトリ作成
        if (!fs::exists(fill_dir)) {
            fs::create_directory(fill_dir);
            std::cout << "[ackit] ディレクトリ作成: " << fill_dir_name << "/\n";
        }

        // 各コンテストのファイルを作成
        for (int n = from_num; n <= to_num; ++n) {
            std::string contest = "abc" + std::to_string(n);
            fs::path cpp_path = fill_dir / (contest + "_" + prob + ".cpp");
            if (fs::exists(cpp_path)) {
                std::cout << "  [skip] " << cpp_path.filename().string() << " (すでに存在)\n";
                continue;
            }
            std::ofstream ofs(cpp_path);
            ofs << tmpl_content;
            std::cout << "  [作成] " << cpp_path.filename().string() << "\n";
        }

        std::cout << "[ackit] 完了: " << fill_dir_name << "/ に "
                  << (to_num - from_num + 1) << " 問分作成しました\n";
        return 0;

    } else if (subcmd == "fill") {
        if (argc < 4) {
            std::cerr << "[ackit] エラー: ackit fill make/test <prob> ...\n";
            return 1;
        }
        return cmd_fill(argc - 2, argv + 2);
    } else if (subcmd == "test") {
        if (argc < 3) {
            std::cerr << "[ackit] エラー: ackit fill test <prob> <contest>\n";
            return 1;
        }
        std::string contest_lower = to_lower(argv[2]);

        if (!fs::exists(fill_dir)) {
            std::cerr << "[ackit] エラー: " << fill_dir_name << "/ が見つかりません\n";
            std::cerr << "           先に ackit fill make を実行してください\n";
            return 1;
        }

        fs::path src = fill_dir / (contest_lower + "_" + prob + ".cpp");
        if (!fs::exists(src)) {
            std::cerr << "[ackit] エラー: " << src.filename().string() << " が見つかりません\n";
            return 1;
        }

        return run_test(src, contest_lower, prob, fill_dir);

    } else {
        std::cerr << "[ackit] エラー: fill のサブコマンドは make / test です\n";
        return 1;
    }
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
                  << "  ackit remove <contest>   例: ackit remove abc300\n"
                  << "  ackit fill make <prob> <from> <to>   例: ackit fill make d abc300 abc400\n"
                  << "  ackit fill test <prob> <contest>      例: ackit fill test d abc300\n";
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
    } else if (subcmd == "fill") {
        if (argc < 4) {
            std::cerr << "[ackit] エラー: ackit fill make/test <prob> ...\n";
            return 1;
        }
        return cmd_fill(argc - 2, argv + 2);
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
