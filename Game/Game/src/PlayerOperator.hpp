
# pragma once

# include <Siv3D.hpp>

// プレイヤーの操作
class PlayerOperator
{
public:

	virtual void update();

	virtual bool isMoveL() const;

	virtual bool isMoveR() const;

	virtual bool isHold() const;

	virtual bool isDrop() const;

	virtual bool isDecide() const;
};
