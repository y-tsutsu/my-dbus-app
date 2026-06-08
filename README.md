# sdbus-c++ D-Bus Demo

このデモでは、D-BusのIDLであるXMLを先に定義し、`sdbus-c++-xml2cpp` でサーバ側adaptorとクライアント側proxyのC++ヘッダを生成します。

## 内容

- `dbus/com.example.Demo.xml`: D-Busインターフェース定義
- `src/server.cpp`: `com.example.DemoService` をsession busへ公開するサーバ
- `src/client.cpp`: サーバのメソッドを呼び、シグナルを受け取るクライアント

公開するD-Bus APIは次の通りです。

- `echo(s message) -> s reply`
- `add(i left, i right) -> i sum`
- `openTempFile() -> h fd`
- `echoed(s message, u callCount)` signal

`h` はD-BusのUNIX file descriptor型です。`openTempFile` はサーバが `/tmp/sdbus-cpp-demo-fd.txt` を開き、そのfdをクライアントへ渡します。クライアントは受け取ったfdへ `write(2)` し、`lseek(2)` で先頭に戻してから `read(2)` します。

`sdbus::UnixFd` はfdを所有するRAII型なので、サーバ側・クライアント側ともに明示的な `close(2)` は不要です。`sdbus::UnixFd` がスコープを抜けるとfdは自動でcloseされます。

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

続けてクライアントは `openTempFile` でfdを受け取り、そのfdを使って `/tmp/sdbus-cpp-demo-fd.txt` に直接読み書きします。
実行後も `/tmp/sdbus-cpp-demo-fd.txt` は削除しないため、あとから `cat /tmp/sdbus-cpp-demo-fd.txt` などで中身を確認できます。

実行例:

```console
method echo reply: server received: hello dbus
signal echoed: message="hello dbus", callCount=1
method add reply : 20 + 22 = 42
method openTempFile fd content: hello via fd
```

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
