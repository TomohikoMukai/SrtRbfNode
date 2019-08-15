# KrigingNode for Maya
このMaya用ノードは、トランスフォームノードの間の連動関係を、与えられたお手本データ（例示データ）を用いて学習します。詳しいアルゴリズムは非公開ですが、ガウス過程モデル（Gaussian process models）と呼ばれる回帰モデル（regression）の一種であるクリギング（kriging）を応用しています。

## 特徴：
- 入力データはローカル行列、出力データは平行移動・回転・スケールです。
- 多対一の制御が可能です。つまり、複数のトランスフォームを入力として、1つのトランスフォームノードを制御できます。

## 動作環境
Windows 10 + Maya 2018（無印）の環境でのみ開発・動作確認しています。他バージョンについては未サポートです。

## 使い方
1. プラグインマネージャーから「KrigingNode.mll」をロードします。
2. 制御される側のトランスフォームノードを選択状態にしたうえで、MELコマンド「CreateKrigingNode」を実行します。実行が成功すると、KrigingNodeが生成され、Targetメッセージアトリビュートが制御先ノードに接続されます。

![CreateKrigingNode](https://github.com/TomohikoMukai/KrigingNode/blob/image/CreateKrigingNode.png)

3. 制御する側のトランスフォームの「Matrix」アトリビュートを、KrigingNodeの「Input」アトリビュートに接続します。

4. サンプルとして与えたいトランスフォームの組合せを指定し、KrigingNodeを選択状態にした上で、MELコマンド「AddKrigingSample」を実行します。実行が成功すると、KrigingNodeのアトリビュート「Examples」のカウントが1つ増えます。

![AddKrigingSample](https://github.com/TomohikoMukai/KrigingNode/blob/image/AddKrigingSample.png)

![KrigingNodeAttributes](https://github.com/TomohikoMukai/KrigingNode/blob/image/KrigingNodeAttributes.png)

5. 必要数のサンプルを追加したら、KrigingNodeの出力を制御先トランスフォームに接続します。あとは制御元トランスフォームに連動して制御先が運動します。

![KrigingNodeDrive](https://github.com/TomohikoMukai/KrigingNode/blob/image/KrigingNodeDrive.png)


## ソースコードについて
現時点では非公開です。将来的にはオープンソースとして公開します。

## 更新履歴
- [2019.8.15] 初版リリース

