# Tiledテストワールド

Tiledで`DirectXGame.tiled-project`を開き、Worldメニューから
`connected-world.world`を読み込んでください。Stage 1は
`rooms/room_01.tmj`から開始し、`rooms/room_02.tmj`へ接続されています。

ゲーム側で使用する規則は次のとおりです。

- `CollisionSolid`上の空でないタイルは、すべて通常ブロックになります。
- `PlayerSpawn`のPoint Objectは、オブジェクト名をスポーンIDとして使います。
- `EnemySpawn`のPoint Objectには`enemyType`プロパティを設定します。使用できる値は
  `WalkEnemy`、`ShieldEnemy`、`FlyingEnemy`です。
- `RoomExit`のRectangle Objectには`targetRoom`と`targetSpawn`が必要です。
- `Checkpoint`のPoint Objectは、オブジェクト名を永続IDとして使います。

Point Objectはタイル中央へ配置してください。Tiledのピクセル座標は
「1タイル＝ゲーム内1ワールド単位」へ変換され、Y軸は上下反転されます。
