# Tiledテストワールド

Tiledで`DirectXGame.tiled-project`を開き、Worldメニューから
`connected-world.world`を読み込んでください。Stage 1は
`rooms/room_01.tmj`から開始し、`rooms/room_02.tmj`、`rooms/room_03.tmj`へ
接続されています。

ゲーム側で使用する規則は次のとおりです。

Tiledが表示するタイルセット画像は`tilesets/collision-solid.png`と
`tilesets/gimmicks.png`です。SVGは編集用の元データなので、図柄を変えた場合は
対応するPNGも更新してください。

- `CollisionSolid`上の空でないタイルは、すべて通常ブロックになります。
- `PlayerSpawn`のPoint Objectは、オブジェクト名をスポーンIDとして使います。
- `EnemySpawn`のPoint Objectには`enemyType`プロパティを設定します。使用できる値は
  `WalkEnemy`、`ShieldEnemy`、`FlyingEnemy`です。
- 3部屋とも`Gimmicks` Tile Layerにギミックを配置しています。追加する場合は
  このレイヤーを選び、`GameplayGimmicks`タイルセットから種類を選んでブラシで塗ります。
  削除は消しゴムを使います。タイル画像はTiled上で種類を見分けるためのもので、
  ゲーム内では従来のギミックモデルを表示します。タイルの`gimmickType`プロパティで
  種類を判定するため、マップ側でIDを直接指定する必要はありません。
  アイコンの略号は`FJ`=`FountainJump`、`MP`=`MovingPlatform`、
  `WL`=`WaterWheelLift`、`UP`=`Updraft`、`BL`=`BounceLeaf`、
  `FR`=`FallingRock`、`VN`=`Vine`、`OS`=`OreSwitch`、
  `SB`=`StoneBridge`、`VC`=`VanishingCloud`、`TC`=`ThunderCloud`、
  `AR`=`AirRushRing`、`AS`=`AirRushSwitch`です。`AR`は空中でプレイヤー本体が触れると、突進が残っていても取得されます。
  突進の保持数は最大1回です。取得時は16個の立方体と8枚の四角板、拡大する円形の波が広がり、リングは消えます。
  プレイヤーが着地すると再出現します。
  `OreSwitch`は攻撃判定、`AirRushSwitch`は突進中の攻撃判定だけで起動します。
  `AirRushSwitch`は空中に置く水色の立方体で、起動中は黄色になります。
  スイッチと`StoneBridge`の対応は`SwitchLinks` Object Layerで指定します。
  Tiledの矩形オブジェクトを各スイッチと石橋タイルの上に重ね、同じ組の
  オブジェクトに文字列プロパティ`linkId`を同じ値で設定してください。
  例えば`Switch 1`と`Walls 1`を`ore_path`、`Switch 2`と`Walls 2`を
  `air_path`にします。1つの矩形で複数の石橋タイルを覆えます。
  全てのスイッチと石橋にリンクが必要で、リンクごとに少なくとも1個の
  スイッチと石橋が必要です。別の`linkId`の石橋は出現しません。
  各リンクの有効時間は約5秒（ゲーム内時間）で、部屋を移動するとリセットされます。
  以前の`GimmickSpawn` Point Objectも
  読み込みには対応していますが、現在の3部屋では使用していません。
  現在は`room_01`に`FountainJump`・`MovingPlatform`・`Updraft`・`Vine`・`AirRushRing`、
  `room_02`に`OreSwitch`・`AirRushSwitch`・`StoneBridge`・`BounceLeaf`・`FallingRock`、
  `room_03`に`WaterWheelLift`・`VanishingCloud`・`ThunderCloud`を配置しています。
  `WaterWheelLift`のタイルは水車の中心軸を示します。中心から半径約2タイルを
  4枚の足場が回るため、周囲約6×5タイルには壁や他のギミックを重ねないでください。
  ツルはプレイヤー本体が接触している間にEキー（ゲームパッドはBボタン）を押すと掴めます。
  ツルからはSpaceキー（ゲームパッドはAボタン）で飛び離れます。
- `RoomExit`のRectangle Objectには`targetRoom`と`targetSpawn`が必要です。
- `Checkpoint`のPoint Objectは、オブジェクト名を永続IDとして使います。
- `Goal`は`Goals` Object Layerの回転なしRectangle Objectです。部屋に1個だけ置けます。
  `room_03`では下側の消える雲の上にある金色の輪がゴールです。触れるとクリア演出後に
  `ClearScene`へ進みます。矩形の位置・大きさをTiledで変更すると判定と輪の位置も変わります。
- `TriangleTrace`のPolygon Objectは、水平な底辺と中央の頂点を持つ
  二等辺三角形を3点で指定します。斜辺は45度で、各辺は1回の突進で届く
  長さにしてください。オブジェクト名は部屋内で固有のIDにします。
  図形ギミックを使わないステージには、このObjectを置かなくて構いません。

三角形は最初、薄い輪郭として表示されます。未描画の辺が残る頂点の近くで
Spaceを押すと頂点に吸い付きます。Spaceを押しながら方向キーで8方向から
自分で方向を選び、Spaceを離す（または選択時間が切れる）と突進します。
未描画の辺と方向が合わなければ突進せず、頂点に留まって選び直せます。
頂点で方向選択中のカーソルは、なぞれる方向なら水色、それ以外なら赤です。
輪郭に沿って次の頂点へ到達すると、そこに吸い付いたまま空中突進が回復します。
各頂点でSpaceを押して次の辺へ進み、途中でやめたい場合は頂点でも突進中でも
Eキー（ゲームパッドはBボタン）で離脱します。突進中に離脱した辺は描画済みになりません。
三辺をすべてなぞると吸い付きが解除され、三角形が
実体化し、斜辺へ上から着地したときに跳ね上がります。完成状態は
部屋を移動しても維持されます。

Point Objectはタイル中央へ配置してください。Tiledのピクセル座標は
「1タイル＝ゲーム内1ワールド単位」へ変換され、Y軸は上下反転されます。
