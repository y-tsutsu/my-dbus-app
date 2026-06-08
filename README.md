# sdbus-c++ D-Bus Demo

Qtに依存しないC++向けD-Busサンプルです。

このデモでは、D-BusのIDLであるXMLを先に定義し、`sdbus-c++-xml2cpp` でサーバ側adaptorとクライアント側proxyのC++ヘッダを生成します。

## 内容

- `dbus/com.example.Demo.xml`: D-Busインターフェース定義
- `src/server.cpp`: `com.example.DemoService` をsession busへ公開するサーバ
- `src/client.cpp`: サーバのメソッドを呼び、シグナルを受け取るクライアント

公開するD-Bus APIは次の通りです。

- `echo(s message) -> s reply`
- `add(i left, i right) -> i sum`
- `echoed(s message, u callCount)` signal

## 依存関係

Debian/Ubuntu系では、環境によって次のようなパッケージが必要です。

```console
sudo apt install cmake g++ libsystemd-dev libsdbus-c++-dev libsdbus-c++-bin
```

ディストリビューションによって `sdbus-c++-xml2cpp` を含むパッケージ名は異なることがあります。

## ビルド

```console
cmake -S . -B build
cmake --build build
```

## 実行

端末を2つ開きます。

1つ目でサーバを起動します。

```console
./build/demo-server
```

2つ目でクライアントを実行します。

```console
./build/demo-client "hello dbus"
```

クライアントは `echo` と `add` メソッドを呼びます。サーバは `echo` の処理中に `echoed` シグナルを送ります。

## busctlで確認する例

サーバ起動中に、別端末からD-Bus上のオブジェクトを確認できます。

```console
busctl --user introspect com.example.DemoService /com/example/Demo
```

メソッド呼び出しもできます。

```console
busctl --user call com.example.DemoService /com/example/Demo com.example.Demo echo s "from busctl"
busctl --user call com.example.DemoService /com/example/Demo com.example.Demo add ii 1 2
```
