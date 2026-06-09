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
- `registerCallback(o callbackPath)`
- `echoed(s message, u callCount)` signal

callback用に、クライアント側は次のAPIも公開します。

- `onServerMessage(s message) -> s reply`

`h` はD-BusのUNIX file descriptor型です。`openTempFile` はサーバが `/tmp/sdbus-cpp-demo-fd.txt` を開き、そのfdをクライアントへ渡します。クライアントは受け取ったfdへ `write(2)` し、`lseek(2)` で先頭に戻してから `read(2)` します。

`sdbus::UnixFd` はfdを所有するRAII型なので、サーバ側・クライアント側ともに明示的な `close(2)` は不要です。`sdbus::UnixFd` がスコープを抜けるとfdは自動でcloseされます。

`registerCallback` は、クライアントが公開したcallback object pathをサーバへ渡すメソッドです。サーバは呼び出し元のsender名とobject pathを覚え、あとからクライアント側の `onServerMessage` をD-Bus method callとして呼びます。

## 依存関係

Debian/Ubuntu系では、環境によって次のようなパッケージが必要です。

```console
$ sudo apt install cmake g++ libsystemd-dev libsdbus-c++-dev libsdbus-c++-bin
```

ディストリビューションによって `sdbus-c++-xml2cpp` を含むパッケージ名は異なることがあります。

## ビルド

```console
$ cmake -S . -B build
$ cmake --build build
```

## 実行

端末を2つ開きます。

1つ目でサーバを起動します。

```console
$ ./build/demo-server
```

2つ目でクライアントを実行します。

```console
$ ./build/demo-client "hello dbus"
```

クライアントは `echo` と `add` メソッドを呼びます。サーバは `echo` の処理中に `echoed` シグナルを送ります。

続けてクライアントは `openTempFile` でfdを受け取り、そのfdを使って `/tmp/sdbus-cpp-demo-fd.txt` に直接読み書きします。
実行後も `/tmp/sdbus-cpp-demo-fd.txt` は削除しないため、あとから `cat /tmp/sdbus-cpp-demo-fd.txt` などで中身を確認できます。

最後にクライアントはcallback objectを `/com/example/DemoCallback` に公開し、そのpathを `registerCallback` でサーバへ渡します。サーバは登録後に、クライアントの `onServerMessage` を呼び返します。

実行例:

```console
method echo reply: server received: hello dbus
signal echoed: message="hello dbus", callCount=1
method add reply : 20 + 22 = 42
method openTempFile fd content: hello via fd
method registerCallback called; waiting for server callback...
callback onServerMessage: hello from server callback
```

## busctlで確認する例

サーバ起動中に、別端末からD-Bus上のオブジェクトを確認できます。
これは `org.freedesktop.DBus.Introspectable.Introspect` を呼び、対象オブジェクトが公開しているinterface、method、signalなどを表示するコマンドです。

```console
$ busctl --user introspect com.example.DemoService /com/example/Demo
```

メソッド呼び出しもできます。

```console
$ busctl --user call com.example.DemoService /com/example/Demo com.example.Demo echo s "from busctl"
$ busctl --user call com.example.DemoService /com/example/Demo com.example.Demo add ii 1 2
```
