
#include "Player.hpp"

Player::Player(Array<int32> dropCells1LoopNum)
	: field(Point(6, 12))
	, just10times(field.getSize())
	, fieldMoveTo(field.getSize())
	, obstructsSentCntArray(field.getSize().x)
{
	for (int32 i : step(static_cast<int32>(dropCells1LoopNum.size())))
	{
		CellType type = static_cast<CellType>(i);
		for (auto j : step(dropCells1LoopNum.at(i)))
		{
			dropCells1LoopCells << Cell(type);
		}
	}

	makeDropCells(10);
}

Player& Player::operator=(const Player& another)
{
	this->dropCells1LoopCells = another.dropCells1LoopCells;
	this->dropCellStack = another.dropCellStack;

	this->field = another.field;
	this->just10times = another.just10times;
	this->fieldMoveTo = another.fieldMoveTo;

	this->fieldColor = another.fieldColor;

	this->dropCellTimer = another.dropCellTimer;
	this->dropCellFieldX = another.dropCellFieldX;

	this->holdCell = another.holdCell;

	this->canOperate = another.canOperate;
	this->canDrop = another.canDrop;

	this->isDeletingTime = another.isDeletingTime;
	this->deletingTimer = another.deletingTimer;

	this->isMovingTime = another.isMovingTime;
	this->movingTimer = another.movingTimer;

	this->stateTimer = another.stateTimer;

	this->state = another.state;

	this->score = another.score;
	this->scoreFunc = another.scoreFunc;
	this->combo = another.combo;

	this->obstructsMakedCnt = another.obstructsMakedCnt;
	this->obstructsSentSum = another.obstructsSentSum;
	this->obstructsSentCntArray = another.obstructsSentCntArray;

	this->sendingObstructCnt = another.sendingObstructCnt;
	this->sendingObstructTimer = another.sendingObstructTimer;

	return *this;
}

void Player::update(PlayerKeySet keySet)
{
	const Size size = field.getSize();
	const Size drawsize = field.getDrawsize();

	const double deltaTime = Scene::DeltaTime();

	// 常に10個以上確保しておく（Next表示対策）
	makeDropCells(10);

	if (KeyTab.down())
	{
		updatedField();
	}

	// 操作可能
	if (canOperate)
	{
		// オジャマを生成する
		if (canDrop)
		{
			makeObstruct();
		}

		// セルをホールドする
		if (keySet.hold.down())
		{
			if (holdCell.getType() == CellType::Empty)
			{
				holdCell = getDropCell(0);
				dropCellStack.remove_at(0);
			}
			else
			{
				Cell tmp = holdCell;
				holdCell = dropCellStack.at(0);
				dropCellStack.at(0) = tmp;
			}
		}

		// 落とすセルを移動する
		if (keySet.moveL.pressed() || keySet.moveR.pressed())
		{
			dropCellTimer += deltaTime;
		}
		if (keySet.moveL.pressed() && dropCellFieldX > 0)
		{
			if (dropCellTimer > dropCellCoolTime)
			{
				dropCellFieldX--;
				dropCellTimer = 0.0;
			}
		}
		if (keySet.moveR.pressed() && dropCellFieldX < drawsize.x - 1)
		{
			if (dropCellTimer > dropCellCoolTime)
			{
				dropCellFieldX++;
				dropCellTimer = 0.0;
			}
		}
		if (keySet.moveL.up() || keySet.moveR.up())
		{
			dropCellTimer = dropCellCoolTime;
		}

		//セルを落下させる
		if (canDrop && keySet.drop.pressed())
		{
			field.pushTopCell(getDropCell(0), dropCellFieldX);

			// 落下処理へ移行
			canDrop = false;
			isMovingTime = true;
			fieldMoveTo = field.getFallTo();

			dropCellStack.remove_at(0);
		}
	}

	// 演出処理
	{
		// セルが消える演出
		if (isDeletingTime)
		{
			deletingTimer += deltaTime;

			if (deletingTimer > deletingCoolTime)
			{
				// 有効Just10Timesの合計
				int32 dc = 0;
				for (auto p : step(drawsize))
				{
					if (field.getField().at(p).getType() != CellType::Obstruct)
					{
						dc += just10times.at(p);
					}
				}

				field.deleteCells(just10times);

				// コンボ倍率においては要調整
				int32 scoreAdd = scoreFunc(dc, combo);
				score += scoreAdd;
				obstructsMakedCnt += scoreAdd;
				combo++;

				isDeletingTime = false;
				deletingTimer = 0.0;

				// 落下処理へ移行
				isMovingTime = true;
				fieldMoveTo = field.getFallTo();
			}
		}

		// セルが動く演出
		if (isMovingTime)
		{
			movingTimer += deltaTime;

			if (movingTimer > movingCoolTime)
			{
				field.moveCells(fieldMoveTo);
				isMovingTime = false;
				movingTimer = 0.0;

				// 消えるものがないなら、動かせるようにする
				if (!updatedField())
				{
					canDrop = true;
					combo = 0;
					//ret = obstructsMaked;
					sendingObstructCnt += obstructsMakedCnt;
					obstructsMakedCnt = 0;
				}
			}
		}

		// オジャマを送る演出
		if (sendingObstructCnt > 0)
		{
			sendingObstructTimer += deltaTime;

			if (isSendObstruct)
			{
				isSendObstruct = false;
				sendingObstructCnt = 0;
				sendingObstructTimer = 0.0;
			}

			if (sendingObstructTimer > sendingObstructCoolTime)
			{
				isSendObstruct = true;
				sendingObstructTimer = 0.0;
			}
		}

		// 負けと表示する演出
		if (state != BattleState::playing)
		{
			stateTimer += deltaTime;
		}
	}
}

void Player::draw(Point fieldPos, Size cellSize, FieldDrawMode drawMode) const
{
	const Size drawsize = field.getDrawsize();

	const Point textMoved = (Vec2(0, -1.5) * cellSize).asPoint();

	// Nextセルの描画
	{
		const Point nextPos =
			drawMode == FieldDrawMode::Left ? fieldPos + Point(-2, 1) * cellSize :
			drawMode == FieldDrawMode::Right ? fieldPos + Point(drawsize.x + 2, 1) * cellSize :
			Point();
		const Point nextPosDiff =
			drawMode == FieldDrawMode::Left ? (Vec2(0.5, -0.5) * cellSize).asPoint() :
			drawMode == FieldDrawMode::Right ? (Vec2(-0.5, -0.5) * cellSize).asPoint() :
			Point();
		FontAsset(U"Text")(U"Next").drawAt(nextPos.movedBy(textMoved).movedBy(2, 3), ColorF(0.0, 0.4));
		FontAsset(U"Text")(U"Next").drawAt(nextPos.movedBy(textMoved), ColorF(0.2));
		Rect(Arg::center(nextPos), cellSize * 2).drawShadow(Vec2(9, 15), 10.0, 0.0, ColorF(0.0, 0.4));
		for (auto i : step(2, 4))
		{
			Rect(Arg::center(nextPos + nextPosDiff + i * Point(0, cellSize.y + 4)), cellSize).drawShadow(Vec2(9, 15), 10.0, 0.0, ColorF(0.0, 0.4));
		}
		getDropCellNotAdd(1).getTexture().resized(cellSize * 2).drawAt(nextPos);
		for (auto i : step(2, 4))
		{
			getDropCellNotAdd(i).getTexture().resized(cellSize).drawAt(nextPos + nextPosDiff + i * Point(0, cellSize.y + 4));
		}
	}

	// ホールドの表示
	{
		const Point holdPos =
			drawMode == FieldDrawMode::Left ? fieldPos + Point(drawsize.x + 2, 1) * cellSize :
			drawMode == FieldDrawMode::Right ? fieldPos + Point(-2, 1) * cellSize :
			Point();
		const Texture hold = holdCell.getType() == CellType::Empty ? Cell::getTexture(CellType::None) : holdCell.getTexture();
		FontAsset(U"Text")(U"Hold").drawAt(holdPos.movedBy(textMoved).movedBy(2, 3), ColorF(0.0, 0.4));
		FontAsset(U"Text")(U"Hold").drawAt(holdPos.movedBy(textMoved), ColorF(0.2));
		Rect(Arg::center(holdPos), cellSize * 2).drawShadow(Vec2(9, 15), 10.0, 0.0, ColorF(0.0, 0.4));
		hold.resized(cellSize * 2).drawAt(holdPos);
	}

	// フィールドの背景と枠
	Rect(fieldPos - cellSize * Size(0, 1), (drawsize + Size(0, 1)) * cellSize)
		.drawShadow(Vec2(9, 15), 10.0, 10.0, ColorF(0.0, 0.4))
		.draw(fieldColor)
		.drawFrame(0.0, 10.0, ColorF(0.2));

	// Just10の回数の表示
	if (KeyControl.pressed())
	{
		for (auto p : step(drawsize))
		{
			Point pos = fieldPos + cellSize * p + cellSize / 2;
			FontAsset(U"Text")(U"{}"_fmt(just10times.at(p))).drawAt(pos, Palette::Black);
		}
	}
	// フィールドの描画
	else
	{
		for (int32 x : step(drawsize.x))
		{
			Point pos = fieldPos + Point(x, 0) * cellSize;

			Cell::getTexture(CellType::No).resized(cellSize).draw(pos);
		}

		field.draw(fieldPos, cellSize,
			[&](Point p, CellType) {
				if (fieldMoveTo.at(p) == Point(-1, -1))
				{
					return (p * cellSize);
				}
				else
				{
					const double e = EaseOutCubic(
						isMovingTime ? movingTimer / movingCoolTime :
						isDeletingTime ? deletingTimer / deletingCoolTime :
						0);
					return (Vec2(p * cellSize).lerp(Vec2(fieldMoveTo.at(p) * cellSize), e)).asPoint();
				}
			},
			[&](Point p, CellType) {
				if (just10times.at(p))
				{
					const double e = EaseOutCubic(deletingTimer / deletingCoolTime);
					return Color(255, static_cast<int32>(255 * (1.0 - e)));
				}
				else
				{
					return Color(255);
				}
			}
			);
	}

	// 落とすセルの描画
	{
		const Point dropCellPos(fieldPos + Point(cellSize.x * dropCellFieldX, -cellSize.y));

		getDropCellNotAdd(0).getTexture().resized(cellSize).draw(dropCellPos);
	}
}

void Player::makeDropCells(int32 min)
{
	// dropCellsを追加
	while (min >= dropCellStack.size())
	{
		dropCellStack.append(dropCells1LoopCells.shuffled());
	}
}

bool Player::updatedField()
{
	const Size drawsize = field.getDrawsize();

	just10times = field.getJust10Times();

	// 消えるセルの周りにあるオジャマも消す
	for (auto p : step(drawsize))
	{
		if (field.getField().at(p).getType() == CellType::Obstruct)
		{
			for (auto i : step(1, 4, 2))
			{
				Point padd = Point(i / 3 - 1, i % 3 - 1);
				if (0 <= p.x + padd.x && p.x + padd.x < drawsize.x
					&& 0 <= p.y + padd.y && p.y + padd.y < drawsize.y
					&& field.getField().at(p + padd).getType() != CellType::Obstruct
					&& just10times.at(p + padd) > 0)
				{
					just10times.at(p) = 1;
				}
			}
		}
	}

	// Just10で消えるものがあるなら、消える時間にする
	if (just10times.any([](int32 n) { return n != 0; }))
	{
		canDrop = false;
		isDeletingTime = true;

		return true;
	}

	// 一番上のyにセルがあるなら、負け
	for (auto x : step(drawsize.x))
	{
		if (field.getField().at(0, x).getType() != CellType::Empty)
		{
			canOperate = false;
			state = BattleState::lose;
			return false;
		}
	}

	return false;
}

void Player::sentObstructs(double sent_obstructs)
{
	const Size size = field.getDrawsize();

	int32 obstructs = static_cast<int32>(ceil(sent_obstructs));

	obstructsSentSum += obstructs;
	for (auto i : step(size.x))
	{
		obstructsSentCntArray.at(i) += obstructs / size.x;
	}

	obstructs %= size.x;

	Array<int32> put(size.x, 0);
	for (auto i : step(size.x))
	{
		put.at(i) = i;
	}

	for (auto i : put.choice(obstructs))
	{
		++obstructsSentCntArray.at(i);
	}
}

void Player::makeObstruct()
{
	const auto size = field.getSize();
	const auto drawsize = field.getDrawsize();

	if (obstructsSentSum > 0)
	{
		for (auto x : step(size.x))
		{
			for (auto y : step(drawsize.y, obstructsSentCntArray.at(x)))
			{
				Cell cell(CellType::Obstruct);
				field.setCell(cell, x, y);
			}
		}

		fieldMoveTo = field.getFloatTo(obstructsSentCntArray);

		obstructsSentSum = 0;
		for (auto x : step(drawsize.x))
		{
			obstructsSentCntArray.at(x) = 0;
		}

		// セル移動処理へ移行
		canDrop = false;
		isMovingTime = true;
	}
}
