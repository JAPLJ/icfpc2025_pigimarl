# Kagamiz C++ ソルバー

これはICFPC 2025用のPython kagamizソルバーのC++移植版です。

## 依存関係

以下のライブラリが必要です：

- **cpp-httplib**: HTTPクライアントライブラリ
- **OpenSSL**: 暗号化機能（SHA256用）
- **yaml-cpp**: YAML設定ファイル解析
- **nlohmann/json**: JSONシリアライゼーション/デシリアライゼーション
- **cxxopts**: コマンドライン引数解析

### macOSでのインストール

```bash
brew install openssl yaml-cpp
```

### Ubuntu/Debianでのインストール

```bash
sudo apt-get install libssl-dev libyaml-cpp-dev
```

## ビルド

### CMakeプリセットを使用（推奨）

```bash
# デフォルト設定でビルド
cmake --preset default
cmake --build --preset default

# デバッグビルド
cmake --preset debug
cmake --build --preset debug

# Ninjaを使用
cmake --preset ninja
cmake --build --preset ninja
```

### 手動ビルド

```bash
mkdir build
cd build
cmake ..
make
```

## 設定

1. 設定ファイルをコピーします：
```bash
cp config.yaml.sample config.yaml
```

2. `config.yaml`を編集して、APIエンドポイント、認証トークン、タイムアウト設定を変更します：
```yaml
api_domain:
  local: http://0.0.0.0:4567
  production: https://your-api-endpoint.com/

token: YOUR_ACTUAL_TOKEN_HERE

request_timeout: 10
```

## 実行

```bash
# ローカル環境でprobatio問題を解く
./kagamiz_cpp --problem probatio --env local

# 本番環境でprimus問題を解く
./kagamiz_cpp --problem primus --env production

# ヘルプを表示
./kagamiz_cpp --help
```

### コマンドライン引数

- `--problem, -p`: 問題名（必須）
  - 選択肢: `probatio`, `primus`, `secundus`, `tertius`, `quartus`, `quintus`
- `--env, -e`: 環境（オプション、デフォルト: `local`）
  - 選択肢: `local`, `production`
- `--help, -h`: ヘルプを表示

## アーキテクチャ

- `schema.hpp/cpp`: データ構造とJSONシリアライゼーション
- `solver.hpp/cpp`: 迷路探索アルゴリズム（DFS）
- `api_handler.hpp/cpp`: HTTP API通信
- `main.cpp`: メインアプリケーションロジック

## 外部ライブラリ

- **cpp-httplib**: HTTP通信
- **OpenSSL**: 暗号化（SHA256ハッシュ）
- **yaml-cpp**: YAML設定ファイル解析
- **nlohmann/json**: JSONシリアライゼーション
- **cxxopts**: コマンドライン引数解析

## トラブルシューティング

### ビルドエラー

- 依存関係がインストールされていることを確認してください
- CMakeのバージョンが3.20以上であることを確認してください

### 実行時エラー

- `config.yaml`が正しく設定されていることを確認してください
- ネットワーク接続とAPIエンドポイントの可用性を確認してください