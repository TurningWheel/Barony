// GameUI.cpp

#include "GameUI.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"
#include "Text.hpp"
#include "Slider.hpp"
#include "Widget.hpp"

#include "../main.hpp"
#include "../game.hpp"
#include "../menu.hpp"
#include "../interface/interface.hpp"
#include "../stat.hpp"
#include "../player.hpp"
#include "../draw.hpp"
#include "../items.hpp"
#include "../mod_tools.hpp"
#include "../input.hpp"
#include "../collision.hpp"
#include "../monster.hpp"
#include "../classdescriptions.hpp"

#include <assert.h>

bool newui = true;
int selectedCursorOpacity = 255;
int oldSelectedCursorOpacity = 255;
int hotbarSlotOpacity = 255;
int hotbarSelectedSlotOpacity = 255;
bool bUsePreciseFieldTextReflow = true;
bool bUseSelectedSlotCycleAnimation = false; // probably not gonna use, but can enable
struct CustomColors_t
{
	Uint32 itemContextMenuHeadingText = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionText = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionSelectedText = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionImg = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionSelectedImg = 0xFFFFFFFF;
	Uint32 characterSheetNeutral = 0xFFFFFFFF;
	Uint32 characterSheetGreen = 0xFFFFFFFF;
	Uint32 characterSheetRed = 0xFFFFFFFF;
} hudColors;
EnemyBarSettings_t enemyBarSettings;

void capitalizeString(std::string& str)
{
	if ( str.size() < 1 ) { return; }
	char letter = str[0];
	if ( letter >= 'a' && letter <= 'z' )
	{
		str[0] = toupper(letter);
	}
}

std::string EnemyBarSettings_t::getEnemyBarSpriteName(Entity* entity)
{
	if ( !entity ) { return "default"; }

	if ( entity->behavior == &actPlayer || entity->behavior == &actMonster )
	{
		int type = entity->getMonsterTypeFromSprite();
		if ( type < NUMMONSTERS && type >= 0 )
		{
			return monstertypename[type];
		}
	}
	else if ( entity->behavior == &actDoor )
	{
		return "door";
	}
	else if ( entity->behavior == &actChest )
	{
		return "chest";
	}
	else if ( entity->behavior == &actFurniture )
	{
		switch ( entity->furnitureType )
		{
			case FURNITURE_TABLE:
			case FURNITURE_PODIUM:
				return "table";
				break;
			default:
				return "chair";
				break;
		}
	}
	return "default";
}

enum ImageIndexes9x9 : int
{
	TOP_LEFT,
	TOP_RIGHT,
	TOP,
	MIDDLE_LEFT,
	MIDDLE_RIGHT,
	MIDDLE,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	BOTTOM
};

const std::vector<std::string> skillsheetEffectBackgroundImages = 
{
	"9x9 bg top left",
	"9x9 bg top right",
	"9x9 bg top middle",
	"9x9 bg left",
	"9x9 bg right",
	"9x9 bg middle",
	"9x9 bg bottom left",
	"9x9 bg bottom right",
	"9x9 bg bottom middle"
};

void imageSetWidthHeight9x9(Frame* container, const std::vector<std::string>& imgNames)
{
	for ( auto& img : imgNames )
	{
		if ( auto i = container->findImage(img.c_str()) )
		{
			if ( auto imgGet = Image::get(i->path.c_str()) )
			{
				i->pos.w = imgGet->getWidth();
				i->pos.h = imgGet->getHeight();
			}
		}
	}
}

// for 9x9 images stretched to fit a container
void imageResizeToContainer9x9(Frame* container, SDL_Rect dimensionsToFill, const std::vector<std::string>& imgNames)
{
	assert(imgNames.size() == 9);
	// adjust inner background image elements
	auto tl = container->findImage(imgNames[TOP_LEFT].c_str());
	tl->pos.x = dimensionsToFill.x;
	tl->pos.y = dimensionsToFill.y;
	auto tr = container->findImage(imgNames[TOP_RIGHT].c_str());
	tr->pos.x = dimensionsToFill.w - tr->pos.w;
	tr->pos.y = dimensionsToFill.y;
	auto tm = container->findImage(imgNames[TOP].c_str());
	tm->pos.x = tl->pos.x + tl->pos.w;
	tm->pos.y = dimensionsToFill.y;
	tm->pos.w = dimensionsToFill.w - tr->pos.w - tl->pos.w;
	auto bl = container->findImage(imgNames[BOTTOM_LEFT].c_str());
	bl->pos.x = dimensionsToFill.x;
	bl->pos.y = dimensionsToFill.h - bl->pos.h;
	auto br = container->findImage(imgNames[BOTTOM_RIGHT].c_str());
	br->pos.x = dimensionsToFill.w - br->pos.w;
	br->pos.y = bl->pos.y;
	auto bm = container->findImage(imgNames[BOTTOM].c_str());
	bm->pos.x = bl->pos.x + bl->pos.w;
	bm->pos.w = dimensionsToFill.w - bl->pos.w - br->pos.w;
	bm->pos.y = bl->pos.y;
	auto ml = container->findImage(imgNames[MIDDLE_LEFT].c_str());
	ml->pos.x = dimensionsToFill.x;
	ml->pos.y = tl->pos.y + tl->pos.h;
	ml->pos.h = dimensionsToFill.h - dimensionsToFill.y - bl->pos.h - tl->pos.h;
	auto mr = container->findImage(imgNames[MIDDLE_RIGHT].c_str());
	mr->pos.x = dimensionsToFill.w - mr->pos.w;
	mr->pos.y = ml->pos.y;
	mr->pos.h = ml->pos.h;
	auto mm = container->findImage(imgNames[MIDDLE].c_str());
	mm->pos.x = ml->pos.x + ml->pos.w;
	mm->pos.y = ml->pos.y;
	mm->pos.w = dimensionsToFill.w - ml->pos.w - mr->pos.w;
	mm->pos.h = ml->pos.h;
}

void createHPMPBars(const int player)
{
	auto& hud_t = players[player]->hud;
	const int barTotalHeight = hud_t.HPMP_FRAME_HEIGHT;
	const int hpBarStartY = (hud_t.hudFrame->getSize().h - hud_t.HPMP_FRAME_START_Y);
	const int mpBarStartY = hpBarStartY + barTotalHeight;
	const int barWidth = hud_t.HPMP_FRAME_WIDTH;
	const int barStartX = hud_t.HPMP_FRAME_START_X;
	{
		hud_t.hpFrame = hud_t.hudFrame->addFrame("hp bar");
		hud_t.hpFrame->setHollow(true);

		SDL_Rect pos{ barStartX, hpBarStartY, barWidth, barTotalHeight };
		hud_t.hpFrame->setSize(pos);

		auto fadeFrame = hud_t.hpFrame->addFrame("hp fade frame");
		fadeFrame->setSize(SDL_Rect{0, 0, barWidth, barTotalHeight});
		fadeFrame->setInheritParentFrameOpacity(false);

		auto foregroundFrame = hud_t.hpFrame->addFrame("hp foreground frame");
		foregroundFrame->setSize(SDL_Rect{ 0, 0, barWidth, barTotalHeight });


		auto base = hud_t.hpFrame->addImage(SDL_Rect{ 54, 4, barWidth - 54, 26 }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_Base_00.png", "hp img base");

		const int progressBarHeight = 22;
		auto fadeProgressBase = fadeFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPMidFade_00.png", "hp img fade bot");
		auto fadeProgress = fadeFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPMidFade_00.png", "hp img fade");
		auto fadeProgressEndCap = fadeFrame->addImage(SDL_Rect{
			fadeProgress->pos.x + fadeProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPEndFade_00.png", "hp img fade endcap");

		auto numbase = foregroundFrame->addImage(SDL_Rect{ 0, 4, 48, 26 }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPNumBase_00.png", "hp img value");
		auto div = foregroundFrame->addImage(SDL_Rect{ 46, 0, 8, 34 }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_Separator_00.png", "hp img div");

		auto currentProgressBase = foregroundFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPBot_00.png", "hp img progress bot");
		auto currentProgress = foregroundFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPMid_00.png", "hp img progress");
		auto currentProgressEndCap = foregroundFrame->addImage(SDL_Rect{
			currentProgress->pos.x + currentProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_00.png", "hp img progress endcap");

		const int endCapWidth = 16;
		auto endCap = foregroundFrame->addImage(SDL_Rect{ pos.w - endCapWidth, 0, endCapWidth, barTotalHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_EndCap_00.png", "hp img endcap");

		auto font = "fonts/pixel_maz.ttf#32#2";
		auto hptext = foregroundFrame->addField("hp text", 16);
		hptext->setText("0");
		hptext->setSize(numbase->pos);
		hptext->setFont(font);
		hptext->setVJustify(Field::justify_t::CENTER);
		hptext->setHJustify(Field::justify_t::CENTER);
		hptext->setColor(SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255));
	}

	// MP bar below
	{
		hud_t.mpFrame = hud_t.hudFrame->addFrame("mp bar");
		hud_t.mpFrame->setHollow(true);

		SDL_Rect pos{ barStartX, mpBarStartY, barWidth, barTotalHeight };
		hud_t.mpFrame->setSize(pos);

		auto fadeFrame = hud_t.mpFrame->addFrame("mp fade frame");
		fadeFrame->setSize(SDL_Rect{ 0, 0, barWidth, barTotalHeight });
		fadeFrame->setInheritParentFrameOpacity(false);

		auto foregroundFrame = hud_t.mpFrame->addFrame("mp foreground frame");
		foregroundFrame->setSize(SDL_Rect{ 0, 0, barWidth, barTotalHeight });


		auto base = hud_t.mpFrame->addImage(SDL_Rect{ 54, 4, barWidth - 54, 26 }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_Base_00.png", "mp img base");

		const int progressBarHeight = 22;
		auto fadeProgressBase = fadeFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPMidFade_00.png", "mp img fade bot");
		auto fadeProgress = fadeFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPMidFade_00.png", "mp img fade");
		auto fadeProgressEndCap = fadeFrame->addImage(SDL_Rect{
			fadeProgress->pos.x + fadeProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPEndFade_00.png", "mp img fade endcap");

		auto numbase = foregroundFrame->addImage(SDL_Rect{ 0, 4, 48, 26 }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPNumBase_00.png", "mp img value");
		auto div = foregroundFrame->addImage(SDL_Rect{ 46, 0, 8, 34 }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_Separator_00.png", "mp img div");

		auto currentProgressBase = foregroundFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPBot_00.png", "mp img progress bot");
		auto currentProgress = foregroundFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPMid_00.png", "mp img progress");
		auto currentProgressEndCap = foregroundFrame->addImage(SDL_Rect{
			currentProgress->pos.x + currentProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_00.png", "mp img progress endcap");

		const int endCapWidth = 16;
		auto endCap = foregroundFrame->addImage(SDL_Rect{ pos.w - endCapWidth, 0, endCapWidth, barTotalHeight }, 0xFFFFFFFF,
			"images/ui/HUD/hpmpbars/HUD_Bars_EndCap_00.png", "mp img endcap");

		auto font = "fonts/pixel_maz.ttf#32#2";
		auto mptext = foregroundFrame->addField("mp text", 16);
		mptext->setText("0");
		mptext->setSize(numbase->pos);
		mptext->setFont(font);
		mptext->setVJustify(Field::justify_t::CENTER);
		mptext->setHJustify(Field::justify_t::CENTER);
		mptext->setColor(SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255));
	}
}

void createEnemyBar(const int player, Frame*& frame)
{
	auto& hud_t = players[player]->hud;
	frame = hud_t.hudFrame->addFrame("enemy bar");
	frame->setHollow(true);
	frame->setInheritParentFrameOpacity(false);
	frame->setOpacity(0.0);
	const int barTotalHeight = hud_t.ENEMYBAR_FRAME_HEIGHT;
	const int barStartY = (hud_t.hudFrame->getSize().h - hud_t.ENEMYBAR_FRAME_START_Y- 100);
	const int barWidth = hud_t.ENEMYBAR_FRAME_WIDTH;

	SDL_Rect pos{ (hud_t.hudFrame->getSize().w / 2) - barWidth / 2 - 6, barStartY, barWidth, barTotalHeight };
	frame->setSize(pos);

	auto bg = frame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Back_Body_01.png", "base img");
	bg->pos.x = 6;
	bg->pos.h = 34;
	bg->pos.y = 4;
	bg->pos.w = 548;
	bg->color = makeColor(255, 255, 255, 255);

	auto bgEndCap = frame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Back_Cap_01.png", "base img endcap");
	bgEndCap->pos.x = bg->pos.x + bg->pos.w;
	bgEndCap->pos.h = bg->pos.h;
	bgEndCap->pos.y = bg->pos.y;
	bgEndCap->pos.w = 8;
	bgEndCap->color = bg->color;

	auto dmgFrame = frame->addFrame("bar dmg frame");
	dmgFrame->setSize(SDL_Rect{ bg->pos.x, bg->pos.y, bg->pos.w + bgEndCap->pos.w, 34 });
	dmgFrame->setInheritParentFrameOpacity(false);
	auto dmg = dmgFrame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_DMG_Body_01.png", "dmg img");
	dmg->pos.x = 0;
	dmg->pos.h = bg->pos.h;
	dmg->pos.y = 0;
	dmg->pos.w = 548 / 2 + 100;
	dmg->color = makeColor(255, 255, 255, 224);

	auto dmgEndCap = dmgFrame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_DMG_Cap_01.png", "dmg img endcap");
	dmgEndCap->pos.x = dmg->pos.w;
	dmgEndCap->pos.h = dmg->pos.h;
	dmgEndCap->pos.y = 0;
	dmgEndCap->pos.w = 10;
	dmgEndCap->color = dmg->color;

	/*auto bubbles = dmgFrame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Bubbles_00.png", "img bubbles");
	bubbles->pos.x = 0;
	bubbles->pos.h = 20;
	bubbles->pos.y = dmgFrame->getSize().h / 2 - bubbles->pos.h / 2;
	if ( auto img = Image::get(bubbles->path.c_str()) )
	{
		bubbles->pos.w = img->getWidth();
	}*/

	auto progressFrame = frame->addFrame("bar progress frame");
	progressFrame->setSize(SDL_Rect{ bg->pos.x, bg->pos.y + 2, bg->pos.w + bgEndCap->pos.w, 30 });
	progressFrame->setOpacity(100.0);
	auto fg = progressFrame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Fill_Body_00.png", "progress img");
	fg->pos.x = 0;
	fg->pos.h = bg->pos.h - 4;
	fg->pos.y = 0;
	fg->pos.w = 548;

	auto fgEndCap = progressFrame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Fill_Cap_00.png", "progress img endcap");
	fgEndCap->pos.x = fg->pos.x + fg->pos.w;
	fgEndCap->pos.h = fg->pos.h;
	fgEndCap->pos.y = 0;
	if ( auto img = Image::get(fgEndCap->path.c_str()) )
	{
		fgEndCap->pos.w = img->getWidth();
	}

	auto skullFrame = frame->addFrame("skull frame");
	skullFrame->setSize(SDL_Rect{ 0, 0, 24, 44 });
	auto skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Face4_00.png", "skull 0 img");
	skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Face3_00.png", "skull 25 img");
	skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Face2_00.png", "skull 50 img");
	skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "images/ui/HUD/enemybar/HUD_EnemyHP_Face1_00.png", "skull 100 img");

	std::string font = "fonts/pixel_maz.ttf#16#2";
	Uint32 color = makeColor(235, 191, 140, 255);
	auto enemyName = frame->addField("enemy name txt", 128);
	enemyName->setSize(SDL_Rect{ 0, 0, frame->getSize().w, frame->getSize().h});
	enemyName->setFont(font.c_str());
	enemyName->setColor(color);
	enemyName->setHJustify(Field::justify_t::CENTER);
	enemyName->setVJustify(Field::justify_t::CENTER);
	enemyName->setText("Skeleton");
	enemyName->setOntop(true);

	auto dmgText = hud_t.hudFrame->addField("enemy dmg txt", 128);
	dmgText->setSize(SDL_Rect{ 0, 0, 0, 0 });
	dmgText->setFont("fonts/pixel_maz.ttf#32#2");
	dmgText->setColor(color);
	dmgText->setHJustify(Field::justify_t::LEFT);
	dmgText->setVJustify(Field::justify_t::TOP);
	dmgText->setText("0");
	dmgText->setOntop(true);
}

void createXPBar(const int player)
{
	auto& hud_t = players[player]->hud;
	hud_t.xpFrame = hud_t.hudFrame->addFrame("xp bar");
	hud_t.xpFrame->setHollow(true);

	const int xpBarStartY = (hud_t.hudFrame->getSize().h) - hud_t.XP_FRAME_START_Y;
	const int xpBarWidth = hud_t.XP_FRAME_WIDTH;
	const int xpBarTotalHeight = hud_t.XP_FRAME_HEIGHT;
	SDL_Rect pos { (hud_t.hudFrame->getSize().w / 2) - xpBarWidth / 2, xpBarStartY, xpBarWidth, xpBarTotalHeight };
	hud_t.xpFrame->setSize(pos);

	auto bg = hud_t.xpFrame->addImage(pos, 0xFFFFFFFF, "images/ui/HUD/xpbar/HUD_Bars_Base_00.png", "xp img base");
	bg->pos.x = 0;
	bg->pos.h = 26;
	bg->pos.y = 4;

	// xpProgress only adjusts width
	const int progressBarHeight = 22;
	auto xpProgress = hud_t.xpFrame->addImage(SDL_Rect{ 0, 6, 1, progressBarHeight }, 0xFFFFFFFF,
		"images/ui/HUD/xpbar/HUD_Bars_ExpMid_00.png", "xp img progress");

	// xpProgressEndCap only adjusts x position based on xpProgress->pos.x + xpProgress->pos.w
	auto xpProgressEndCap = hud_t.xpFrame->addImage(SDL_Rect{0, 6, 8, progressBarHeight }, 0xFFFFFFFF,
		"images/ui/HUD/xpbar/HUD_Bars_ExpEnd_00.png", "xp img progress endcap");

	const int endCapWidth = 26;
	SDL_Rect endCapPos {0, 0, endCapWidth, xpBarTotalHeight};
	auto endCapLeft = hud_t.xpFrame->addImage(endCapPos, 0xFFFFFFFF, "images/ui/HUD/xpbar/HUD_Bars_ExpCap1_00.png", "xp img endcap left");
	endCapPos.x = pos.w - endCapPos.w;
	auto endCapRight = hud_t.xpFrame->addImage(endCapPos, 0xFFFFFFFF, "images/ui/HUD/xpbar/HUD_Bars_ExpCap2_00.png", "xp img endcap right");

	const int textWidth = 40;
	auto font = "fonts/pixel_maz.ttf#32#2";
	auto textStatic = hud_t.xpFrame->addField("xp text static", 16);
	textStatic->setText("/100");
	textStatic->setSize(SDL_Rect{ pos.w / 2 - 4, 0, textWidth, pos.h }); // x - 4 to center the slash
	textStatic->setFont(font);
	textStatic->setVJustify(Field::justify_t::CENTER);
	textStatic->setHJustify(Field::justify_t::LEFT);
	textStatic->setColor(SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255));

	auto text = hud_t.xpFrame->addField("xp text current", 16);
	text->setText("0");
	text->setSize(SDL_Rect{ pos.w / 2 - (4 * 2) - textWidth, 0, textWidth, pos.h }); // x - 4 to center the slash
	text->setFont(font);
	text->setVJustify(Field::justify_t::CENTER);
	text->setHJustify(Field::justify_t::RIGHT);
	text->setColor(SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255));
}

void createHotbar(const int player)
{
	auto& hotbar_t = players[player]->hotbar;
	if ( !hotbar_t.hotbarFrame )
	{
		return;
	}
	Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, hotbarSlotOpacity);
	SDL_Rect slotPos{ 0, 0, hotbar_t.getSlotSize(), hotbar_t.getSlotSize() };
	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		char slotname[32];
		snprintf(slotname, sizeof(slotname), "hotbar slot %d", i);
		auto slot = hotbar_t.hotbarFrame->addFrame(slotname);
		slot->setSize(slotPos);
		slot->addImage(slotPos, color, "images/ui/HUD/hotbar/HUD_Quickbar_Slot_Box_00.png", "slot img");

		char glyphname[32];
		snprintf(glyphname, sizeof(glyphname), "hotbar glyph %d", i);
		auto glyph = hotbar_t.hotbarFrame->addImage(slotPos, 0xFFFFFFFF, "images/ui/Glyphs/G_Switch_A00.png", glyphname);
		glyph->disabled = true;
	}

	auto font = "fonts/pixel_maz.ttf#32#2";

	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		char slotname[32];
		snprintf(slotname, sizeof(slotname), "hotbar slot %d", i);
		auto slot = hotbar_t.hotbarFrame->findFrame(slotname);
		assert(slot);

		auto itemSlot = slot->addFrame("hotbar slot item");
		SDL_Rect itemSlotTempSize = slot->getSize();
		itemSlotTempSize.w -= 2;
		itemSlotTempSize.h -= 2;
		itemSlot->setSize(itemSlotTempSize); // slightly shrink to align inner item elements within rect.
		createPlayerInventorySlotFrameElements(itemSlot);
		itemSlot->setSize(slot->getSize());

		char numStr[4];
		snprintf(numStr, sizeof(numStr), "%d", i + 1);
		auto text = slot->addField("slot num text", 4);
		text->setText(numStr);
		text->setSize(SDL_Rect{ 0, -4, slotPos.w, slotPos.h });
		text->setFont(font);
		text->setVJustify(Field::justify_t::TOP);
		text->setHJustify(Field::justify_t::LEFT);
		text->setOntop(true);
	}

	auto highlightFrame = hotbar_t.hotbarFrame->addFrame("hotbar highlight");
	highlightFrame->setSize(slotPos);
	highlightFrame->addImage(slotPos, color, "images/ui/HUD/hotbar/HUD_Quickbar_Slot_HighlightBox_00.png", "highlight img");

	auto itemSlot = highlightFrame->addFrame("hotbar slot item");
	SDL_Rect itemSlotTempSize = slotPos;
	itemSlotTempSize.w -= 2;
	itemSlotTempSize.h -= 2;
	itemSlot->setSize(itemSlotTempSize);
	createPlayerInventorySlotFrameElements(itemSlot); // slightly shrink to align inner item elements within rect.
	itemSlot->setSize(highlightFrame->getSize());

	{
		auto oldSelectedFrame = hotbar_t.hotbarFrame->addFrame("hotbar old selected item");
		oldSelectedFrame->setSize(slotPos);
		oldSelectedFrame->setDisabled(true);

		SDL_Rect itemSpriteBorder{ 2, 2, oldSelectedFrame->getSize().w - 4, oldSelectedFrame->getSize().h - 4 };

		color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
		auto oldImg = oldSelectedFrame->addImage(itemSpriteBorder,
			SDL_MapRGBA(mainsurface->format, 255, 255, 255, 128), "", "hotbar old selected item");
		oldImg->disabled = true;
		oldSelectedFrame->addImage(SDL_Rect{ 0, 0, oldSelectedFrame->getSize().w, oldSelectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "hotbar old selected highlight");

		auto oldCursorFrame = hotbar_t.hotbarFrame->addFrame("hotbar old item cursor");
		oldCursorFrame->setSize(SDL_Rect{ 0, 0, slotPos.w + 16, slotPos.h + 16 });
		oldCursorFrame->setDisabled(true);
		color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, oldSelectedCursorOpacity);
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_TL.png", "hotbar old cursor topleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_TR.png", "hotbar old cursor topright");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_BL.png", "hotbar old cursor bottomleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_BR.png", "hotbar old cursor bottomright");
	}

	{
		auto cursorFrame = hotbar_t.hotbarFrame->addFrame("shootmode selected item cursor");
		cursorFrame->setSize(SDL_Rect{ 0, 0, slotPos.w + 16, slotPos.h + 16 });
		cursorFrame->setDisabled(true);
		color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, selectedCursorOpacity);
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_TL.png", "shootmode selected cursor topleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_TR.png", "shootmode selected cursor topright");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_BL.png", "shootmode selected cursor bottomleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_BR.png", "shootmode selected cursor bottomright");
	}

	auto text = highlightFrame->addField("slot num text", 4);
	text->setText("");
	text->setSize(SDL_Rect{ 0, -4, slotPos.w, slotPos.h });
	text->setFont(font);
	text->setVJustify(Field::justify_t::TOP);
	text->setHJustify(Field::justify_t::LEFT);
	text->setOntop(true);
}

void Player::HUD_t::processHUD()
{
	char name[32];
	snprintf(name, sizeof(name), "player hud %d", player.playernum);
	if ( !hudFrame )
	{
		hudFrame = gui->addFrame(name);
		hudFrame->setHollow(true);
		hudFrame->setBorder(0);
		hudFrame->setOwner(player.playernum);
	}
	hudFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	if ( gamePaused || nohud || !players[player.playernum]->isLocalPlayer() )
	{
		// hide
		hudFrame->setDisabled(true);
	}
	else
	{
		hudFrame->setDisabled(false);
	}

	if ( !xpFrame )
	{
		createXPBar(player.playernum);
	}
	if ( !hpFrame )
	{
		createHPMPBars(player.playernum);
	}
	if ( !enemyBarFrame )
	{
		createEnemyBar(player.playernum, enemyBarFrame);
	}
	if ( !enemyBarFrameHUD )
	{
		createEnemyBar(player.playernum, enemyBarFrameHUD);
	}
	updateXPBar();
	updateHPBar();
	updateMPBar();
	enemyHPDamageBarHandler[player.playernum].cullExpiredHPBars();
	enemyBarFrame->setDisabled(true);
	enemyBarFrameHUD->setDisabled(true);
	for ( auto& HPBar : enemyHPDamageBarHandler[player.playernum].HPBars )
	{
		updateEnemyBar2(enemyBarFrame, &HPBar.second);
	}
}

void Player::MessageZone_t::createChatbox()
{
	char name[32];
	snprintf(name, sizeof(name), "player chat %d", player.playernum);
	if ( !gui->findFrame(name) )
	{
		Frame* chatMainFrame = gui->addFrame(name);
		chatMainFrame->setHollow(true);
		chatMainFrame->setBorder(0);
		chatMainFrame->setOwner(player.playernum);
		chatMainFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
			players[player.playernum]->camera_virtualy1(),
			Frame::virtualScreenX,
			Frame::virtualScreenY });
		chatFrame = chatMainFrame;
		Frame* messages = chatMainFrame->addFrame("message box");
		messages->setSize(SDL_Rect{ 224, 16, 
			players[player.playernum]->camera_virtualWidth() / 2,
			players[player.playernum]->camera_virtualHeight() });

		static const char* bigfont = "fonts/pixelmix.ttf#16#2";
		SDL_Rect entryPos{ 0, 0, messages->getSize().w, 0 };
		for ( int i = 0; i < MESSAGE_MAX_ENTRIES; ++i )
		{
			char msgName[32];
			snprintf(msgName, sizeof(msgName), "message %d", i);
			auto entry = messages->addField(msgName, ADD_MESSAGE_BUFFER_LENGTH);
			entry->setFont(bigfont);
			entry->setSize(entryPos);
			entry->setDisabled(true);
		}
	}
}

void Player::MessageZone_t::processChatbox()
{
	if ( !chatFrame )
	{
		createChatbox();
	}

	chatFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	const int leftAlignedBottomY = 484;
	const int topAlignedBottomY = 216;
	int topAlignedPaddingX = 224;
	chatboxTopAlignedPos = SDL_Rect{ topAlignedPaddingX, 0,
		players[player.playernum]->camera_virtualWidth() - topAlignedPaddingX * 2,
		players[player.playernum]->camera_virtualHeight() };
	chatboxLeftAlignedPos = SDL_Rect{ 8, 0, players[player.playernum]->camera_virtualWidth() / 2,
		players[player.playernum]->camera_virtualHeight() };

	Frame* messageBoxFrame = chatFrame->findFrame("message box");
	if (gamePaused) {
		messageBoxFrame->setDisabled(true);
	} else {
		messageBoxFrame->setDisabled(false);
	}
	SDL_Rect messageBoxSize = messageBoxFrame->getSize();
	if ( player.shootmode && messageBoxSize.x == chatboxTopAlignedPos.x )
	{
		chatboxMovedResize = true;
	}
	else if ( !player.shootmode && messageBoxSize.x == chatboxLeftAlignedPos.x )
	{
		chatboxMovedResize = true;
	}

	if ( chatboxMovedResize )
	{
		if ( player.shootmode )
		{
			messageBoxSize = chatboxLeftAlignedPos;
		}
		else
		{
			messageBoxSize = chatboxTopAlignedPos;
		}
	}

	char msgName[32];
	for ( int i = 0; i < MESSAGE_MAX_ENTRIES; ++i )
	{
		snprintf(msgName, sizeof(msgName), "message %d", i);
		if ( auto entry = messageBoxFrame->findField(msgName) )
		{
			entry->setDisabled(true);
			SDL_Rect entryPos = entry->getSize();
			entryPos.w = messageBoxSize.w;
			entry->setSize(entryPos);
		}
	}

	const int entryPaddingY = 4;
	int currentY = (messageBoxSize.h);
	int index = 0;

	bool messageDrawDescending = false;
	if ( messageDrawDescending )
	{
		currentY = 4;
	}

	for ( Message *current : notification_messages )
	{
		if ( index >= MESSAGE_MAX_ENTRIES )
		{
			break;
		}

		Uint32 color = current->text->color ^ mainsurface->format->Amask;
		color += std::min<Sint16>(std::max<Sint16>(0, current->alpha), 255) << mainsurface->format->Ashift;

		snprintf(msgName, sizeof(msgName), "message %d", index);
		if ( auto entry = messageBoxFrame->findField(msgName) )
		{
			entry->setDisabled(false);
			entry->setColor(color);
			entry->setText(current->text->data);

			if ( current->requiresResize )
			{
				entry->reflowTextToFit(0);
				//current->requiresResize = false;
			}

			Text* textGet = Text::get(entry->getText(), entry->getFont(),
				makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
			if ( !messageDrawDescending )
			{
				currentY -= textGet->getHeight();
			}

			SDL_Rect pos = entry->getSize();
			pos.h = textGet->getHeight();
			pos.y = currentY;
			if ( messageDrawDescending )
			{
				currentY += textGet->getHeight();
			}
			entry->setSize(pos);
			if ( pos.y < 0 )
			{
				entry->setDisabled(true); // clipping outside of frame
			}
		}

		if ( messageDrawDescending )
		{
			currentY += entryPaddingY;
		}
		else
		{
			currentY -= entryPaddingY;
		}
		++index;
	}

	if ( player.shootmode )
	{
		messageBoxSize.y = leftAlignedBottomY - messageBoxSize.h;
	}
	else
	{
		messageBoxSize.y = topAlignedBottomY - messageBoxSize.h;
	}
	if ( messageDrawDescending )
	{
		messageBoxSize.y = 4;
	}
	messageBoxFrame->setSize(messageBoxSize);
	/*int newHeight = messageBoxSize.h - currentY;
	messageBoxSize.h = newHeight;

	if ( currentY < 0 )
	{
		for ( int i = 0; i < MESSAGE_MAX_ENTRIES; ++i )
		{
			snprintf(msgName, sizeof(msgName), "message %d", i);
			if ( auto entry = messageBoxFrame->findField(msgName) )
			{
				SDL_Rect pos = entry->getSize();
				pos.y -= currentY;
				entry->setSize(pos);
			}
		}
	}*/
}

std::map<std::string, std::pair<std::string, std::string>> Player::CharacterSheet_t::mapDisplayNamesDescriptions;
std::string Player::CharacterSheet_t::defaultString = "";
std::map<std::string, std::string> Player::CharacterSheet_t::hoverTextStrings;
void Player::CharacterSheet_t::loadCharacterSheetJSON()
{
	if ( !PHYSFS_getRealDir("/data/charsheet.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/charsheet.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/charsheet.json");
		inputPath.append("/data/charsheet.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);
			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( d.HasMember("level_strings") )
				{
					mapDisplayNamesDescriptions.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["level_strings"].MemberBegin();
						itr != d["level_strings"].MemberEnd(); ++itr )
					{
						std::string name = "";
						std::string desc = "";
						if ( itr->value.HasMember("display_name") )
						{
							name = itr->value["display_name"].GetString();
						}
						if ( itr->value.HasMember("description") )
						{
							desc = itr->value["description"].GetString();
						}
						mapDisplayNamesDescriptions[itr->name.GetString()] = std::make_pair(name, desc);
					}
				}
				if ( d.HasMember("hover_text") )
				{
					hoverTextStrings.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["hover_text"].MemberBegin();
						itr != d["hover_text"].MemberEnd(); ++itr )
					{
						hoverTextStrings[itr->name.GetString()] = itr->value.GetString();
					}
				}
			}
		}
	}
}

void Player::CharacterSheet_t::createCharacterSheet()
{
	char name[32];
	snprintf(name, sizeof(name), "player sheet %d", player.playernum);
	if ( !gui->findFrame(name) )
	{
		Frame* sheetFrame = gui->addFrame(name);
		sheetFrame->setHollow(true);
		sheetFrame->setBorder(0);
		sheetFrame->setOwner(player.playernum);
		sheetFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
			players[player.playernum]->camera_virtualy1(),
			Frame::virtualScreenX,
			Frame::virtualScreenY });
		this->sheetFrame = sheetFrame;

		const int bgWidth = 208;
		const int leftAlignX = sheetFrame->getSize().w - bgWidth;
		{
			Frame* fullscreenBg = sheetFrame->addFrame("sheet bg fullscreen");
			fullscreenBg->setSize(SDL_Rect{ leftAlignX,
				0, 208, Frame::virtualScreenY });
			// if splitscreen 3/4 - disable the fullscreen background + title text.
			fullscreenBg->addImage(SDL_Rect{ 0, 0, fullscreenBg->getSize().w, 360 },
				0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_Window_01A_Top.png", "bg image");

			const char* titleFont = "fonts/pixel_maz.ttf#32#2";
			auto characterSheetTitleText = fullscreenBg->addField("character sheet title text", 32);
			characterSheetTitleText->setFont(titleFont);
			characterSheetTitleText->setSize(SDL_Rect{ 6, 177, 202, 32 });
			characterSheetTitleText->setText("CHARACTER SHEET");
			characterSheetTitleText->setVJustify(Field::justify_t::CENTER);
			characterSheetTitleText->setHJustify(Field::justify_t::CENTER);
		}

		{
			auto tooltipFrame = sheetFrame->addFrame("sheet tooltip");
			tooltipFrame->setSize(SDL_Rect{ leftAlignX - 200, 0, 200, 200 });
			Uint32 color = makeColor(255, 255, 255, 255);
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_T_00.png", skillsheetEffectBackgroundImages[TOP].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_L_00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_R_00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				makeColor(22, 24, 29, 255), "images/system/white.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_BL_00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_BR_00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/CharSheet/HUD_CharSheet_Tooltip_B_00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
			imageSetWidthHeight9x9(tooltipFrame, skillsheetEffectBackgroundImages);
			imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, 200, 200 }, skillsheetEffectBackgroundImages);
			auto txt = tooltipFrame->addField("tooltip text", 1024);
			const char* tooltipFont = "fonts/pixel_maz_multiline.ttf#16#2";
			txt->setFont(tooltipFont);
			txt->setColor(makeColor(188, 154, 114, 255));
			auto glyph1 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 1");
			glyph1->disabled = true;
			auto glyph2 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 2");
			glyph2->disabled = true;
			auto glyph3 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 3");
			glyph3->disabled = true;
			auto glyph4 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 4");
			glyph4->disabled = true;

			auto txtEntry = tooltipFrame->addField("txt 1", 1024);
			txtEntry->setFont(tooltipFont);
			txtEntry->setDisabled(true);
			txtEntry->setVJustify(Field::justify_t::CENTER);
			txtEntry->setColor(makeColor(188, 154, 114, 255));

			txtEntry = tooltipFrame->addField("txt 2", 1024);
			txtEntry->setFont(tooltipFont);
			txtEntry->setDisabled(true);
			txtEntry->setVJustify(Field::justify_t::CENTER);
			txtEntry->setColor(makeColor(188, 154, 114, 255));

			txtEntry = tooltipFrame->addField("txt 3", 1024);
			txtEntry->setFont(tooltipFont);
			txtEntry->setDisabled(true);
			txtEntry->setVJustify(Field::justify_t::CENTER);
			txtEntry->setColor(makeColor(188, 154, 114, 255));

			txtEntry = tooltipFrame->addField("txt 4", 1024);
			txtEntry->setFont(tooltipFont);
			txtEntry->setDisabled(true);
			txtEntry->setVJustify(Field::justify_t::CENTER);
			txtEntry->setColor(makeColor(188, 154, 114, 255));
		}

		// log / map buttons
		{
			const char* buttonFont = "fonts/pixel_maz.ttf#32#2";
			SDL_Rect buttonFramePos{ leftAlignX + 9, 6, 196, 82 };
			auto buttonFrame = sheetFrame->addFrame("log map buttons");
			buttonFrame->setSize(buttonFramePos);
			buttonFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos){
				auto frame = (Frame*)(&widget);
				std::vector<const char*> buttons = {
					"map button",
					"log button"
				};
				for ( auto button : buttons )
				{
					if ( auto b = frame->findButton(button) )
					{
						if ( players[frame->getOwner()]->characterSheet.sheetDisplayType == CHARSHEET_DISPLAY_NORMAL )
						{
							b->setBackgroundHighlighted("images/ui/CharSheet/HUD_CharSheet_ButtonHigh_00.png");
							b->setBackground("images/ui/CharSheet/HUD_CharSheet_Button_00.png");
						}
						else
						{
							b->setBackgroundHighlighted("images/ui/CharSheet/HUD_CharSheet_ButtonHighCompact_00.png");
							b->setBackground("images/ui/CharSheet/HUD_CharSheet_ButtonCompact_00.png");
						}
					}
				}
			});

			SDL_Rect buttonPos{0, 0, buttonFramePos.w, 40};
			auto mapButton = buttonFrame->addButton("map button");
			mapButton->setText(language[4069]);
			mapButton->setFont(buttonFont);
			mapButton->setBackground("images/ui/CharSheet/HUD_CharSheet_Button_00.png");
			mapButton->setSize(buttonPos);
			mapButton->setHideGlyphs(true);
			mapButton->setColor(makeColor(255, 255, 255, 255));
			mapButton->setHighlightColor(makeColor(255, 255, 255, 255));
			mapButton->setCallback([](Button& button){
				messagePlayer(button.getOwner(), "%d: Map button clicked", button.getOwner());
			});
			
			auto mapSelector = buttonFrame->addFrame("map button selector");
			mapSelector->setSize(buttonPos);
			mapSelector->setHollow(true);
			//mapSelector->setClickable(true);

			buttonPos.y = buttonPos.y + buttonPos.h + 2;
			auto logButton = buttonFrame->addButton("log button");
			logButton->setText(language[4070]);
			logButton->setFont(buttonFont);
			logButton->setBackground("images/ui/CharSheet/HUD_CharSheet_Button_00.png");
			logButton->setSize(buttonPos);
			logButton->setHideGlyphs(true);
			logButton->setColor(makeColor(255, 255, 255, 255));
			logButton->setHighlightColor(makeColor(255, 255, 255, 255));
			logButton->setCallback([](Button& button) {
				messagePlayer(button.getOwner(), "%d: Log button clicked", button.getOwner());
			});
			
			auto logSelector = buttonFrame->addFrame("log button selector");
			logSelector->setSize(buttonPos);
			logSelector->setHollow(true);
			//logSelector->setClickable(true);
		}

		// game timer
		{
			const char* timerFont = "fonts/pixel_maz.ttf#32#2";
			Uint32 timerTextColor = SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255);

			Frame* timerFrame = sheetFrame->addFrame("game timer");
			timerFrame->setSize(SDL_Rect{leftAlignX + 36, 90, 142, 26});
			timerFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos) {
				auto frame = (Frame*)(&widget);
				auto timerToggleImg = frame->findImage("timer icon img");
				auto timerSelector = frame->findButton("timer selector");
				if ( timerToggleImg && timerSelector )
				{
					Uint8 tmpAlpha = 128;
					Uint8 tmpRed = 128;
					Uint8 tmpBlue = 128;
					if ( !players[frame->getOwner()]->characterSheet.showGameTimerAlways )
					{
						tmpRed = 255;
						tmpBlue = 255;
					}
					if ( timerSelector->isHighlighted() )
					{
						tmpAlpha = 192;
						timerToggleImg->color = makeColor(tmpRed, 255, tmpBlue, tmpAlpha);
					}
					else
					{
						tmpAlpha = 255;
						timerToggleImg->color = makeColor(tmpRed, 255, tmpBlue, tmpAlpha);
					}
				}
			});
			auto timerToggleImg = timerFrame->addImage(SDL_Rect{0, 0, 26, 26}, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_ButtonArrows_00.png", "timer icon img");
			auto timerImg = timerFrame->addImage(SDL_Rect{ 30, 0, 112, 26 }, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_Timer_Backing_00.png", "timer bg img");
			auto timerText = timerFrame->addField("timer text", 32);
			timerText->setFont(timerFont);

			auto timerButton = timerFrame->addButton("timer selector");
			timerButton->setSize(SDL_Rect{ 0, 0, timerToggleImg->pos.w + timerImg->pos.w, 26 });
			timerButton->setColor(makeColor(0, 0, 0, 0));
			timerButton->setHighlightColor(makeColor(0, 0, 0, 0));
			timerButton->setHideGlyphs(true);
			timerButton->setCallback([](Button& button){
				bool& bShowTimer = players[button.getOwner()]->characterSheet.showGameTimerAlways;
				bShowTimer = !bShowTimer;
			});

			SDL_Rect textPos = timerImg->pos;
			textPos.x += 12;
			timerText->setSize(textPos);
			timerText->setVJustify(Field::justify_t::CENTER);
			timerText->setText("00:92:30:89");
			timerText->setColor(timerTextColor);
		}

		// skills button
		{
			const char* skillsFont = "fonts/pixel_maz.ttf#32#2";
			Frame* skillsButtonFrame = sheetFrame->addFrame("skills button frame");
			skillsButtonFrame->setSize(SDL_Rect{ leftAlignX + 14, 360 - 8 - 42, 186, 42 });
			auto skillsButton = skillsButtonFrame->addButton("skills button");
			skillsButton->setText(language[4074]);
			skillsButton->setFont(skillsFont);
			skillsButton->setBackground("images/ui/CharSheet/HUD_CharSheet_ButtonWide_00.png");
			skillsButton->setSize(SDL_Rect{ 0, 0, skillsButtonFrame->getSize().w, skillsButtonFrame->getSize().h });
			skillsButton->setHideGlyphs(true);
			skillsButton->setColor(makeColor(255, 255, 255, 191));
			skillsButton->setHighlightColor(makeColor(255, 255, 255, 255));
			skillsButton->setCallback([](Button& button) {
				messagePlayer(button.getOwner(), "%d: Skills button clicked", button.getOwner());
				players[button.getOwner()]->skillSheet.openSkillSheet();
			});
			//auto skillsImg = skillsButtonFrame->addImage(SDL_Rect{0, 0, skillsButtonFrame->getSize().w, skillsButtonFrame->getSize().h},
			//	0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_ButtonWide_00.png", "skills button img");
			//auto skillsText = skillsButtonFrame->addField("skills text", 32);
			//skillsText->setFont(skillsFont);
			//skillsText->setSize(skillsImg->pos);
			//skillsText->setVJustify(Field::justify_t::CENTER);
			//skillsText->setHJustify(Field::justify_t::CENTER);
			//skillsText->setText("Skills List");
		}

		// dungeon floor and level descriptor
		{
			const char* dungeonFont = "fonts/pixel_maz.ttf#32#2";
			Uint32 dungeonTextColor = SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255);
			Frame* dungeonFloorFrame = sheetFrame->addFrame("dungeon floor frame");

			dungeonFloorFrame->setSize(SDL_Rect{ leftAlignX + 6, 118, 202, 52 });
			auto dungeonButton = dungeonFloorFrame->addButton("dungeon floor selector");
			dungeonButton->setSize(SDL_Rect{ 6, 6, 190, 44 });
			dungeonButton->setColor(makeColor(0, 0, 0, 0));
			dungeonButton->setHighlightColor(makeColor(0, 0, 0, 0));
			dungeonButton->setHideGlyphs(true);

			auto floorNameText = dungeonFloorFrame->addField("dungeon name text", 32);
			floorNameText->setFont(dungeonFont);
			floorNameText->setSize(SDL_Rect{ 20, 0, 162, 26 });
			floorNameText->setText("Sand Labyrinth");
			floorNameText->setVJustify(Field::justify_t::CENTER);
			floorNameText->setHJustify(Field::justify_t::CENTER);
			floorNameText->setColor(dungeonTextColor);

			auto floorLevelText = dungeonFloorFrame->addField("dungeon level text", 32);
			floorLevelText->setFont(dungeonFont);
			floorLevelText->setSize(SDL_Rect{ 20, 26, 162, 26 });
			floorLevelText->setText("Floor 27");
			floorLevelText->setVJustify(Field::justify_t::CENTER);
			floorLevelText->setHJustify(Field::justify_t::CENTER);
			floorLevelText->setColor(dungeonTextColor);
		}

		Frame* characterFrame = sheetFrame->addFrame("character info");
		const char* infoFont = "fonts/pixel_maz.ttf#32#2";
		characterFrame->setSize(SDL_Rect{ leftAlignX, 206, bgWidth, 116});
		Uint32 infoTextColor = SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255);
		Uint32 classTextColor = SDL_MapRGBA(mainsurface->format, 74, 66, 207, 255);

		sheetFrame->addImage(SDL_Rect{ characterFrame->getSize().x, characterFrame->getSize().y - 60, 
			214, 170 }, 0xFFFFFFFF,
			"images/ui/CharSheet/HUD_CharSheet_Window_01A_TopCompact.png", "character info compact img");

		Frame* characterInnerFrame = characterFrame->addFrame("character info inner frame");
		characterInnerFrame->setSize(SDL_Rect{ 6, 0, 202, 104 });
		{
			//characterInnerFrame->addImage(SDL_Rect{ 0, 0, characterInnerFrame->getSize().w, characterInnerFrame->getSize().h }, 0xFFFFFFFF,
			//	"images/ui/CharSheet/HUD_CharSheet_Window_01A_TopTmp.png", "character info tmp img");


			SDL_Rect characterTextPos{ 2, 0, 198, 24 };
			auto nameText = characterInnerFrame->addField("character name text", 32);
			nameText->setFont(infoFont);
			nameText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			nameText->setText("Slartibartfast");
			nameText->setVJustify(Field::justify_t::CENTER);
			nameText->setHJustify(Field::justify_t::CENTER);
			nameText->setColor(infoTextColor);

			/*characterInnerFrame->addImage(SDL_Rect{ 2, 2, 198, 20 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character name frame");*/
			//characterInnerFrame->addFrame("character name selector")->setSize(SDL_Rect{ 2, 2, 198, 20 });

			characterTextPos.x = 8;
			characterTextPos.w = 190;
			characterTextPos.y = 52;
			characterTextPos.h = 26;
			auto levelText = characterInnerFrame->addField("character level text", 32);
			levelText->setFont(infoFont);
			levelText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			levelText->setText("");
			levelText->setVJustify(Field::justify_t::CENTER);
			levelText->setColor(infoTextColor);

			/*characterInnerFrame->addImage(SDL_Rect{ 4, 54, 194, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character level frame");*/
			auto classButton = characterInnerFrame->addButton("character class selector");
			classButton->setSize(SDL_Rect{ 4, 54, 194, 22 });
			classButton->setColor(makeColor(0, 0, 0, 0));
			classButton->setHighlightColor(makeColor(0, 0, 0, 0));
			classButton->setHideGlyphs(true);

			characterTextPos.x = 8;
			characterTextPos.w = 190;
			characterTextPos.y = 52;
			characterTextPos.h = 26;
			auto classText = characterInnerFrame->addField("character class text", 32);
			classText->setFont(infoFont);
			classText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			classText->setText("Barbarian");
			classText->setVJustify(Field::justify_t::CENTER);
			classText->setHJustify(Field::justify_t::LEFT);
			classText->setColor(classTextColor);

			characterTextPos.x = 4;
			characterTextPos.w = 194;
			characterTextPos.y = 26;
			characterTextPos.h = 26;
			auto raceText = characterInnerFrame->addField("character race text", 32);
			raceText->setFont(infoFont);
			raceText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			raceText->setText("Human Gloomforge");
			raceText->setVJustify(Field::justify_t::CENTER);
			raceText->setHJustify(Field::justify_t::CENTER);
			raceText->setColor(infoTextColor);

			auto sexImg = characterInnerFrame->addImage(
				SDL_Rect{ characterTextPos.x + characterTextPos.w - 40, characterTextPos.y - 4, 16, 28}, 0xFFFFFFFF,
				"images/ui/CharSheet/HUD_CharSheet_Sex_M_01.png", "character sex img");

			/*characterInnerFrame->addImage(SDL_Rect{ 4, 28, 194, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character race frame");*/
			auto raceButton = characterInnerFrame->addButton("character race selector");
			raceButton->setSize(SDL_Rect{ 4, 28, 194, 22 });
			raceButton->setColor(makeColor(0, 0, 0, 0));
			raceButton->setHighlightColor(makeColor(0, 0, 0, 0));
			raceButton->setHideGlyphs(true);

			characterTextPos.x = 4;
			characterTextPos.w = 194;
			characterTextPos.y = 78;
			characterTextPos.h = 26;
			auto goldTitleText = characterInnerFrame->addField("gold text title", 8);
			goldTitleText->setFont(infoFont);
			goldTitleText->setSize(SDL_Rect{ 48, characterTextPos.y, 52, characterTextPos.h });
			goldTitleText->setText("GOLD");
			goldTitleText->setVJustify(Field::justify_t::CENTER);
			goldTitleText->setColor(infoTextColor);

			auto goldText = characterInnerFrame->addField("gold text", 32);
			goldText->setFont(infoFont);
			goldText->setSize(SDL_Rect{ 92, characterTextPos.y, 88, characterTextPos.h });
			goldText->setText("0");
			goldText->setVJustify(Field::justify_t::CENTER);
			goldText->setHJustify(Field::justify_t::CENTER);
			goldText->setColor(infoTextColor);

			auto goldImg = characterInnerFrame->addImage(SDL_Rect{ 24, characterTextPos.y - 4, 20, 28 },
				0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_ButtonMoney_00.png", "gold img");

			/*characterInnerFrame->addImage(SDL_Rect{ 24, 80, 156, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character gold frame");*/
			auto goldButton = characterInnerFrame->addButton("character gold selector");
			goldButton->setSize(SDL_Rect{ 22, 80, 158, 22 });
			goldButton->setColor(makeColor(0, 0, 0, 0));
			goldButton->setHighlightColor(makeColor(0, 0, 0, 0));
			goldButton->setHideGlyphs(true);
		}

		{
			Frame* statsFrame = sheetFrame->addFrame("stats");
			const int statsFrameHeight = 182;
			statsFrame->setSize(SDL_Rect{ leftAlignX, 344, bgWidth, statsFrameHeight });
			statsFrame->addImage(SDL_Rect{ 0, 0, statsFrame->getSize().w, statsFrame->getSize().h },
				0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_Window_01B_BotA.png", "stats bg img");

			Frame* statsInnerFrame = statsFrame->addFrame("stats inner frame");
			statsInnerFrame->setSize(SDL_Rect{ 20, 0, statsFrame->getSize().w, statsFrame->getSize().h });

			SDL_Rect iconPos{ 0, 8, 24, 24 };
			const int headingLeftX = iconPos.x + iconPos.w + 10;
			const int baseStatLeftX = headingLeftX + 32;
			const int modifiedStatLeftX = baseStatLeftX + 64;
			SDL_Rect textPos{ headingLeftX, iconPos.y, 40, iconPos.h };
			Uint32 statTextColor = hudColors.characterSheetNeutral;

			const char* statFont = "fonts/pixel_maz.ttf#32#2";
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_STR_00.png", "str icon");
			{
				auto textBase = statsInnerFrame->addField("str text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("STR");
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("str text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("-1");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("str text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("342");
				textStatModified->setColor(statTextColor);
			}
			const int rowSpacing = 4;
			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_DEX_00.png", "dex icon");
			{
				auto textBase = statsInnerFrame->addField("dex text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("DEX");
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("dex text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("1");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("dex text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("2");
				textStatModified->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_CON_00.png", "con icon");
			{
				auto textBase = statsInnerFrame->addField("con text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("CON");
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("con text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("con text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("-2");
				textStatModified->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_INT_00.png", "int icon");
			{
				auto textBase = statsInnerFrame->addField("int text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("INT");
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("int text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("3");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("int text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_PER_00.png", "per icon");
			{
				auto textBase = statsInnerFrame->addField("per text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("PER");
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("per text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("-400");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("per text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("-299");
				textStatModified->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_CHA_00.png", "chr icon");
			{
				auto textBase = statsInnerFrame->addField("chr text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("CHR");
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("chr text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("2");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("chr text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);
			}
		}

		{
			Frame* attributesFrame = sheetFrame->addFrame("attributes");
			attributesFrame->setSize(SDL_Rect{ leftAlignX, 550, bgWidth, 182 });

			attributesFrame->addImage(SDL_Rect{ 0, 0, attributesFrame->getSize().w, attributesFrame->getSize().h },
				0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_Window_01B_BotB.png", "attributes bg img");

			Frame* attributesInnerFrame = attributesFrame->addFrame("attributes inner frame");
			attributesInnerFrame->setSize(SDL_Rect{ 20, 0, attributesFrame->getSize().w, attributesFrame->getSize().h });

			SDL_Rect iconPos{ 0, 8, 24, 24 };
			const int headingLeftX = iconPos.x + iconPos.w + 10;
			const int baseStatLeftX = headingLeftX + 44;
			SDL_Rect textPos{ headingLeftX, iconPos.y, 80, iconPos.h };
			Uint32 statTextColor = hudColors.characterSheetNeutral;

			const char* attributeFont = "fonts/pixel_maz.ttf#32#2";
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_ATT_00.png", "atk icon");
			{
				auto textBase = attributesInnerFrame->addField("atk text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("ATK");
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("atk text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("14");
				textStat->setColor(statTextColor);
			}

			const int rowSpacing = 4;
			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_AC_00.png", "ac icon");
			{
				auto textBase = attributesInnerFrame->addField("ac text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("AC");
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("ac text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("3");
				textStat->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_SPWR_00.png", "pwr icon");
			{
				auto textBase = attributesInnerFrame->addField("pwr text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("PWR");
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("pwr text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("115%");
				textStat->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_RES_00.png", "res icon");
			{
				auto textBase = attributesInnerFrame->addField("res text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("RES");
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("res text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("100%");
				textStat->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_REGEN_00.png", "regen icon");
			{
				auto textBase = attributesInnerFrame->addField("regen text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("RGN");
				textBase->setColor(statTextColor);

				auto textDiv = attributesInnerFrame->addField("regen text divider", 4);
				textDiv->setVJustify(Field::justify_t::CENTER);
				textDiv->setHJustify(Field::justify_t::CENTER);
				textDiv->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textDiv->setSize(textPos);
				textDiv->setText("/");
				textDiv->setColor(statTextColor);

				SDL_Rect hpmpTextPos = textPos;
				const int middleX = (textPos.x + textPos.w / 2);
				auto textRegenHP = attributesInnerFrame->addField("regen text hp", 16);
				textRegenHP->setVJustify(Field::justify_t::CENTER);
				textRegenHP->setHJustify(Field::justify_t::RIGHT);
				textRegenHP->setFont(attributeFont);
				hpmpTextPos.x = middleX - 4 - hpmpTextPos.w;
				textRegenHP->setSize(hpmpTextPos);
				textRegenHP->setText("0.2");
				textRegenHP->setColor(statTextColor);

				auto textRegenMP = attributesInnerFrame->addField("regen text mp", 16);
				textRegenMP->setVJustify(Field::justify_t::CENTER);
				textRegenMP->setHJustify(Field::justify_t::LEFT);
				textRegenMP->setFont(attributeFont);
				hpmpTextPos.x = middleX + 4;
				textRegenMP->setSize(hpmpTextPos);
				textRegenMP->setText("0.1");
				textRegenMP->setColor(statTextColor);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_WGT_00.png", "weight icon");
			{
				auto textBase = attributesInnerFrame->addField("weight text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText("WGT");
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("weight text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("120");
				textStat->setColor(statTextColor);
			}
		}
	}
}

void Player::CharacterSheet_t::processCharacterSheet()
{
	if ( !sheetFrame )
	{
		createCharacterSheet();
	}

	sheetFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	if ( !stats[player.playernum] || !players[player.playernum]->isLocalPlayer() )
	{
		sheetFrame->setDisabled(true);
		return;
	}

	if ( nohud || player.shootmode || 
		!(player.gui_mode == GUI_MODE_INVENTORY || player.gui_mode == GUI_MODE_SHOP) )
	{
		sheetFrame->setDisabled(true);
		return;
	}
	sheetFrame->setDisabled(false);
	
	// resize elements for splitscreen
	{
		auto characterInfoFrame = sheetFrame->findFrame("character info");
		const int bgWidth = 208;
		const int leftAlignX = sheetFrame->getSize().w - bgWidth;
		const int compactAlignX = leftAlignX - 256;
		auto compactImgBg = sheetFrame->findImage("character info compact img");
		Frame* dungeonFloorFrame = sheetFrame->findFrame("dungeon floor frame");
		Frame* buttonFrame = sheetFrame->findFrame("log map buttons");
		Frame* fullscreenBg = sheetFrame->findFrame("sheet bg fullscreen");
		Frame* timerFrame = sheetFrame->findFrame("game timer");
		Frame* skillsButtonFrame = sheetFrame->findFrame("skills button frame");

		auto statsFrame = sheetFrame->findFrame("stats");
		auto attributesFrame = sheetFrame->findFrame("attributes");

		if ( keystatus[SDL_SCANCODE_U] )
		{
			// compact view
			sheetDisplayType = CHARSHEET_DISPLAY_COMPACT;

			compactImgBg->disabled = false;
			compactImgBg->pos.x = compactAlignX;
			compactImgBg->pos.y = 0;

			SDL_Rect pos = characterInfoFrame->getSize();
			pos.x = compactAlignX;
			pos.y = compactImgBg->pos.y + 60;
			characterInfoFrame->setSize(pos);

			SDL_Rect dungeonFloorFramePos = dungeonFloorFrame->getSize();
			dungeonFloorFramePos.x = pos.x + 6;
			dungeonFloorFramePos.y = pos.y - 56;
			dungeonFloorFrame->setSize(dungeonFloorFramePos);

			SDL_Rect buttonFramePos = buttonFrame->getSize();
			buttonFramePos.x = compactAlignX + 8;
			buttonFramePos.y = compactImgBg->pos.y + compactImgBg->pos.h + 1;
			buttonFramePos.w = 198;
			buttonFramePos.h = 40;
			buttonFrame->setSize(buttonFramePos);
			{
				SDL_Rect buttonPos{ 0, 1, 98, 38 };
				auto mapBtn = buttonFrame->findButton("map button");
				mapBtn->setText(language[4071]);
				mapBtn->setSize(buttonPos);
				auto mapSelector = buttonFrame->findFrame("map button selector");
				SDL_Rect mapSelectorPos = buttonPos;
				mapSelectorPos.x += 6;
				mapSelectorPos.w -= 12;
				mapSelectorPos.y += 4;
				mapSelectorPos.h -= 8;
				mapSelector->setSize(mapSelectorPos);

				buttonPos.x = buttonFramePos.w - buttonPos.w;
				auto logBtn = buttonFrame->findButton("log button");
				logBtn->setText(language[4072]);
				logBtn->setSize(buttonPos);
				auto logSelector = buttonFrame->findFrame("log button selector");
				SDL_Rect logSelectorPos = buttonPos;
				logSelectorPos.x += 6;
				logSelectorPos.w -= 12;
				logSelectorPos.y += 4;
				logSelectorPos.h -= 10;
				logSelector->setSize(logSelectorPos);
			}

			fullscreenBg->setDisabled(true);

			auto statsPos = statsFrame->getSize();
			statsPos.y = 0;
			statsFrame->setSize(statsPos);

			auto attributesPos = attributesFrame->getSize();
			attributesPos.y = statsPos.y + statsPos.h - 4; // 4 pixels above bottom edge of stats pane
			attributesFrame->setSize(attributesPos);
		}
		else
		{
			// standard view
			sheetDisplayType = CHARSHEET_DISPLAY_NORMAL;

			SDL_Rect pos = characterInfoFrame->getSize();
			pos.x = leftAlignX;
			pos.y = 206;
			characterInfoFrame->setSize(pos);

			SDL_Rect dungeonFloorFramePos = dungeonFloorFrame->getSize();
			dungeonFloorFramePos.x = pos.x + 6;
			dungeonFloorFramePos.y = pos.y - 88;
			dungeonFloorFrame->setSize(dungeonFloorFramePos);

			SDL_Rect buttonFramePos = buttonFrame->getSize();
			buttonFramePos.x = leftAlignX + 9;
			buttonFramePos.y = 6;
			buttonFramePos.w = 196;
			buttonFramePos.h = 82;
			buttonFrame->setSize(buttonFramePos);
			{
				SDL_Rect buttonPos{ 0, 0, buttonFramePos.w, 40 };
				auto mapBtn = buttonFrame->findButton("map button");
				mapBtn->setText(language[4069]);
				mapBtn->setSize(buttonPos);
				auto mapSelector = buttonFrame->findFrame("map button selector");
				mapSelector->setSize(buttonPos);

				buttonPos.y = buttonPos.y + buttonPos.h + 2;
				auto logBtn = buttonFrame->findButton("log button");
				logBtn->setText(language[4070]);
				logBtn->setSize(buttonPos);
				auto logSelector = buttonFrame->findFrame("log button selector");
				logSelector->setSize(buttonPos);
			}

			SDL_Rect timerFramePos = timerFrame->getSize();
			timerFramePos.x = leftAlignX + 36;
			timerFrame->setSize(timerFramePos);

			SDL_Rect skillsButtonFramePos = skillsButtonFrame->getSize();
			skillsButtonFramePos.x = leftAlignX + 14;
			skillsButtonFrame->setSize(skillsButtonFramePos);

			fullscreenBg->setDisabled(false);
			SDL_Rect fullscreenBgPos = fullscreenBg->getSize();
			fullscreenBgPos.x = sheetFrame->getSize().w - fullscreenBgPos.w;
			fullscreenBg->setSize(fullscreenBgPos);
			compactImgBg->disabled = true;

			auto attributesPos = attributesFrame->getSize();
			attributesPos.y = sheetFrame->getSize().h - attributesPos.h;
			attributesFrame->setSize(attributesPos);

			auto statsPos = statsFrame->getSize();
			statsPos.y = attributesPos.y - statsPos.h + 4; // 4 pixels below top edge of attributes pane
			statsFrame->setSize(statsPos);
		}
	}

	updateCharacterSheetTooltip(SHEET_UNSELECTED, SDL_Rect{ 0, 0, 0, 0 }); // to reset the tooltip from displaying.
	player.GUI.handleCharacterSheetMovement();

	updateGameTimer();
	updateStats();
	updateAttributes();
	updateCharacterInfo();
}

void Player::CharacterSheet_t::selectElement(SheetElements element, bool usingMouse, bool moveCursor)
{
	selectedElement = element;

	Frame* elementFrame = nullptr;
	Frame::image_t* img = nullptr;
	Field* elementField = nullptr;
	Button* elementButton = nullptr;
	switch ( element )
	{
		case SHEET_OPEN_LOG:
			if ( elementFrame = sheetFrame->findFrame("log map buttons") )
			{
				elementFrame = elementFrame->findFrame("log button selector");
				//img = elementFrame->findImage("log button img");
			}
			break;
		case SHEET_OPEN_MAP:
			if ( elementFrame = sheetFrame->findFrame("log map buttons") )
			{
				elementFrame = elementFrame->findFrame("map button selector");
				//img = elementFrame->findImage("map button img");
			}
			break;
		case SHEET_SKILL_LIST:
			elementFrame = sheetFrame->findFrame("skills button frame");
			break;
		case SHEET_TIMER:
			elementFrame = sheetFrame->findFrame("game timer");
			break;
		case SHEET_GOLD:
			if ( elementFrame = sheetFrame->findFrame("character info") )
			{
				if ( elementFrame = elementFrame->findFrame("character info inner frame") )
				{
					elementButton = elementFrame->findButton("character gold selector");
				}
			}
			break;
		case SHEET_DUNGEON_FLOOR:
			if ( elementFrame = sheetFrame->findFrame("dungeon floor frame") )
			{
				elementButton = elementFrame->findButton("dungeon floor selector");
			}
			break;
		case SHEET_CHAR_CLASS:
			if ( elementFrame = sheetFrame->findFrame("character info") )
			{
				if ( elementFrame = elementFrame->findFrame("character info inner frame") )
				{
					elementButton = elementFrame->findButton("character class selector");
				}
			}
			break;
		case SHEET_CHAR_RACE_SEX:
			if ( elementFrame = sheetFrame->findFrame("character info") )
			{
				if ( elementFrame = elementFrame->findFrame("character info inner frame") )
				{
					elementButton = elementFrame->findButton("character race selector");
				}
			}
			break;
		case SHEET_STR:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				elementField = elementFrame->findField("str text stat");
			}
			break;
		case SHEET_DEX:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				elementField = elementFrame->findField("dex text stat");
			}
			break;
		case SHEET_CON:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				elementField = elementFrame->findField("con text stat");
			}
			break;
		case SHEET_INT:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				elementField = elementFrame->findField("int text stat");
			}
			break;
		case SHEET_PER:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				elementField = elementFrame->findField("per text stat");
			}
			break;
		case SHEET_CHR:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				elementField = elementFrame->findField("chr text stat");
			}
			break;
		case SHEET_ATK:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				elementField = elementFrame->findField("atk text stat");
			}
			break;
		case SHEET_AC:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				elementField = elementFrame->findField("ac text stat");
			}
			break;
		case SHEET_POW:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				elementField = elementFrame->findField("atk text stat");
			}
			break;
		case SHEET_RES:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				elementField = elementFrame->findField("res text stat");
			}
			break;
		case SHEET_RGN:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				elementField = elementFrame->findField("regen text title");
			}
			break;
		case SHEET_WGT:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				elementField = elementFrame->findField("weight text stat");
			}
			break;
		default:
			elementFrame = nullptr;
			break;
	}

	SDL_Rect pos{ 0, 0, 0, 0 };
	if ( elementFrame && moveCursor )
	{
		elementFrame->warpMouseToFrame(player.playernum, (Inputs::SET_CONTROLLER));
		pos = elementFrame->getAbsoluteSize();
		if ( img )
		{
			pos.x += img->pos.x;
			pos.y += img->pos.y;
			pos.w = img->pos.w;
			pos.h = img->pos.h;
		}
		if ( elementButton )
		{
			pos = elementButton->getAbsoluteSize();
		}
		if ( elementField )
		{
			pos = elementField->getAbsoluteSize();
		}
		// make sure to adjust absolute size to camera viewport
		pos.x -= player.camera_virtualx1();
		pos.y -= player.camera_virtualy1();
		player.hud.setCursorDisabled(false);
		player.hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, usingMouse);
	}
}

void Player::CharacterSheet_t::updateGameTimer()
{
	auto timerText = sheetFrame->findFrame("game timer")->findField("timer text");
	char buf[32];

	Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
	Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
	Uint32 hour = ((completionTime / TICKS_PER_SECOND) / 60) / 60;
	Uint32 day = ((completionTime / TICKS_PER_SECOND) / 60) / 60 / 24;
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", day, hour, min, sec);
	timerText->setText(buf);

	if ( selectedElement == SHEET_TIMER )
	{
		updateCharacterSheetTooltip(SHEET_TIMER, sheetFrame->findFrame("game timer")->getSize());
	}
}

std::string& Player::CharacterSheet_t::getHoverTextString(std::string key)
{
	if ( hoverTextStrings.find(key) != hoverTextStrings.end() )
	{
		return hoverTextStrings[key];
	}
	return defaultString;
}

void Player::CharacterSheet_t::updateCharacterSheetTooltip(SheetElements element, SDL_Rect pos)
{
	if ( !sheetFrame )
	{
		return;
	}
	auto tooltipFrame = sheetFrame->findFrame("sheet tooltip");
	if ( element == SHEET_ENUM_END || element == SHEET_UNSELECTED
		|| player.GUI.activeModule != Player::GUI_t::MODULE_CHARACTERSHEET )
	{
		tooltipFrame->setDisabled(true);
		return;
	}
	tooltipFrame->setDisabled(false);

	auto txt = tooltipFrame->findField("tooltip text");

	Uint32 defaultColor = makeColor(188, 154, 114, 255);
	for ( int i = 1; i < 5; ++i )
	{
		char glyphName[32] = "";
		snprintf(glyphName, sizeof(glyphName), "glyph %d", i);
		char entryName[32] = "";
		snprintf(entryName, sizeof(entryName), "txt %d", i);
		auto glyph = tooltipFrame->findImage(glyphName); assert(glyph);
		glyph->disabled = true;
		auto entry = tooltipFrame->findField(entryName); assert(entry);
		entry->setDisabled(true);
		entry->setColor(defaultColor);
	}

	if ( element == Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR )
	{
		const int maxWidth = 260;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		
		txt->setText(mapDisplayNamesDescriptions[map.name].second.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight;
		txt->setSize(txtPos);
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		tooltipPos.w = txtGet->getWidth() + padx * 2;
		tooltipPos.h = pady1 + txtPos.h + pady2;
		tooltipPos.x = pos.x - tooltipPos.w;
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			skillsheetEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_GOLD )
	{
		const int maxWidth = 200;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 8;
		const int padyMid = 4;
		const int padxMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		if ( inputs.getVirtualMouse(player.playernum)->lastMovementFromController )
		{
			txt->setText(getHoverTextString("gold_controller").c_str());
		}
		else
		{
			txt->setText(getHoverTextString("gold_mouse").c_str());
		}
		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + padyMid;
		txt->setSize(txtPos);

		int currentHeight = txtPos.y + txtPos.h;
		for ( int i = 1; i < 5; ++i )
		{
			currentHeight += padyMid;
			char glyphName[32] = "";
			snprintf(glyphName, sizeof(glyphName), "glyph %d", i);
			char entryName[32] = "";
			snprintf(entryName, sizeof(entryName), "txt %d", i);
			auto glyph = tooltipFrame->findImage(glyphName); assert(glyph);
			glyph->disabled = false;
			auto entry = tooltipFrame->findField(entryName); assert(entry);
			entry->setDisabled(false);
			glyph->pos.x = padx;
			glyph->pos.y = currentHeight;
			switch ( i )
			{
				case 1:
					glyph->path = "images/ui/Glyphs/G_Switch_A00.png";
					entry->setText(getHoverTextString("gold_option_10").c_str());
					break;
				case 2:
					glyph->path = "images/ui/Glyphs/G_Switch_B00.png";
					entry->setText(getHoverTextString("gold_option_100").c_str());
					break;
				case 3:
					glyph->path = "images/ui/Glyphs/G_Switch_Y00.png";
					entry->setText(getHoverTextString("gold_option_1000").c_str());
					break;
				case 4:
					glyph->path = "images/ui/Glyphs/G_Switch_X00.png";
					entry->setText(getHoverTextString("gold_option_all").c_str());
					break;
				default:
					break;
			}
			if ( auto imgGet = Image::get(glyph->path.c_str()) )
			{
				glyph->pos.w = imgGet->getWidth();
				glyph->pos.h = imgGet->getHeight();
			}
			SDL_Rect entryPos = entry->getSize();
			entryPos.x = glyph->pos.x + glyph->pos.w + padxMid;
			entryPos.y = glyph->pos.y + glyph->pos.h / 2;
			if ( auto textGet = Text::get(entry->getText(), entry->getFont(), entry->getTextColor(), entry->getOutlineColor()) )
			{
				entryPos.h = textGet->getHeight() * entry->getNumTextLines();
				entryPos.y -= entryPos.h / 2;
				entryPos.w = textGet->getWidth();
			}
			entry->setSize(entryPos);
			currentHeight = std::max(entryPos.y + entryPos.h, glyph->pos.y + glyph->pos.h);
		}

		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		tooltipPos.w = txtGet->getWidth() + padx * 2;
		tooltipPos.h = pady1 + currentHeight + pady2;
		tooltipPos.x = pos.x - tooltipPos.w;
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			skillsheetEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_TIMER )
	{
		const int maxWidth = 200;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 8;
		const int padyMid = 4;
		const int padxMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		if ( inputs.getVirtualMouse(player.playernum)->lastMovementFromController )
		{
			txt->setText(getHoverTextString("game_timer_controller").c_str());
		}
		else
		{
			txt->setText(getHoverTextString("game_timer_mouse").c_str());
		}

		int currentHeight = padyMid;
		SDL_Rect entry1Pos { padx, currentHeight, 0, 0 };
		SDL_Rect entry2Pos { padx, currentHeight, 0, 0 };
		for ( int i = 1; i <= 2; ++i )
		{
			char entryName[32] = "";
			snprintf(entryName, sizeof(entryName), "txt %d", i);
			auto entry = tooltipFrame->findField(entryName); assert(entry);
			entry->setDisabled(false);
			
			switch ( i )
			{
				case 1:
					entry->setText(getHoverTextString("game_timer_state_desc").c_str());
					break;
				case 2:
					if ( showGameTimerAlways )
					{
						entry->setText(getHoverTextString("game_timer_state_enabled").c_str());
						entry->setColor(makeColor(0, 255, 0, 255));
					}
					else
					{
						entry->setText(getHoverTextString("game_timer_state_disabled").c_str());
						entry->setColor(makeColor(255, 0, 0, 255));
					}
					break;
				default:
					break;
			}

			SDL_Rect* entryPos = nullptr;
			if ( i == 1 )
			{
				entryPos = &entry1Pos;
			}
			else
			{
				entryPos = &entry2Pos;
				entry2Pos.x = entry1Pos.x + entry1Pos.w;
			}
			if ( Font* actualFont = Font::get(entry->getFont()) )
			{
				entryPos->h = entry->getNumTextLines() * actualFont->height(true);
			}
			if ( auto textGet = Text::get(entry->getText(), entry->getFont(), entry->getTextColor(), entry->getOutlineColor()) )
			{
				entryPos->w = textGet->getWidth();
			}
			entry->setSize(*entryPos);
		}

		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txtPos.w = std::max(entry1Pos.w + entry2Pos.w, txtPos.w);
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + padyMid;
		txt->setSize(txtPos);
		
		entry1Pos.y += txtPos.h;
		entry2Pos.y += txtPos.h;
		tooltipFrame->findField("txt 1")->setSize(entry1Pos);
		tooltipFrame->findField("txt 2")->setSize(entry2Pos);
		currentHeight = std::max(currentHeight, entry1Pos.y + entry1Pos.h);

		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		tooltipPos.w = txtPos.w + padx * 2;
		tooltipPos.h = pady1 + currentHeight + pady2;
		tooltipPos.x = pos.x - tooltipPos.w;
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			skillsheetEffectBackgroundImages);
	}
}

void Player::CharacterSheet_t::updateCharacterInfo()
{
	auto characterInfoFrame = sheetFrame->findFrame("character info");
	assert(characterInfoFrame);
	auto characterInnerFrame = characterInfoFrame->findFrame("character info inner frame");
	assert(characterInnerFrame);

	SheetElements targetElement = SHEET_ENUM_END;
	auto logButton = sheetFrame->findFrame("log map buttons")->findButton("log button");
	auto mapButton = sheetFrame->findFrame("log map buttons")->findButton("map button");
	auto skillsButton = sheetFrame->findFrame("skills button frame")->findButton("skills button");
	if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		if ( skillsButton->isHighlighted() )
		{
			targetElement = SHEET_SKILL_LIST;
		}
		else if ( logButton->isHighlighted() )
		{
			targetElement = SHEET_OPEN_LOG;
		}
		else if ( mapButton->isHighlighted() )
		{
			targetElement = SHEET_OPEN_MAP;
		}
		else if ( sheetFrame->findFrame("game timer")->findButton("timer selector")->isHighlighted() )
		{
			targetElement = SHEET_TIMER;
		}
		else if ( sheetFrame->findFrame("dungeon floor frame")->findButton("dungeon floor selector")->isHighlighted() )
		{
			targetElement = SHEET_DUNGEON_FLOOR;
		}
		else if ( characterInnerFrame->findButton("character class selector")->isHighlighted() )
		{
			targetElement = SHEET_CHAR_CLASS;
		}
		else if ( characterInnerFrame->findButton("character race selector")->isHighlighted() )
		{
			targetElement = SHEET_CHAR_RACE_SEX;
		}
		else if ( characterInnerFrame->findButton("character gold selector")->isHighlighted() )
		{
			targetElement = SHEET_GOLD;
		}
	}
	if ( targetElement != SHEET_ENUM_END )
	{
		player.GUI.activateModule(Player::GUI_t::MODULE_CHARACTERSHEET);
		selectElement(targetElement, true, true);
	}
	else
	{
		if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			if ( player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET )
			{
				// no moused over objects, deactivate the cursor.
				player.GUI.activateModule(Player::GUI_t::MODULE_NONE);
			}
		}
	}
	//messagePlayer(0, "%d", player.GUI.activeModule);
	// players[player.playernum]->GUI.warpControllerToModule(false); - use this for controller input

	//auto characterPos = characterInfoFrame->getSize();
	//characterPos.x = player.camera_virtualWidth() - characterPos.w * 2;
	//characterInfoFrame->setSize(characterPos);

	char buf[32] = "";
	if ( auto name = characterInnerFrame->findField("character name text") )
	{
		name->setText(stats[player.playernum]->name);
	}
	Field* className = characterInnerFrame->findField("character class text");
	int classNameWidth = 0;
	if ( className )
	{
		std::string classname = playerClassLangEntry(client_classes[player.playernum], player.playernum);
		if ( !classname.empty() )
		{
			capitalizeString(classname);
			className->setText(classname.c_str());
		}
		if ( auto textGet = Text::get(className->getText(), className->getFont(),
			className->getTextColor(), className->getOutlineColor()) )
		{
			classNameWidth = textGet->getWidth();
		}
	}
	Field* charLevel = characterInnerFrame->findField("character level text");
	int charLevelWidth = 0;
	if ( charLevel )
	{
		snprintf(buf, sizeof(buf), language[4051], stats[player.playernum]->LVL);
		charLevel->setText(buf);
		if ( auto textGet = Text::get(charLevel->getText(), charLevel->getFont(),
			charLevel->getTextColor(), charLevel->getOutlineColor()) )
		{
			charLevelWidth = textGet->getWidth();
		}
	}
	if ( className && charLevel )
	{
		SDL_Rect classNamePos = className->getSize();
		SDL_Rect charLevelPos = charLevel->getSize();
		const int padding = 24;
		const int startX = 8;
		const int fieldWidth = 190;
		const int totalWidth = charLevelWidth + padding / 2 + classNameWidth;
		charLevelPos.x = startX + fieldWidth / 2 - totalWidth / 2;
		charLevelPos.w = charLevelWidth;
		classNamePos.x = charLevelPos.x + charLevelPos.w + padding / 2 - 4; // -4 centres it nicely somehow, not sure why
		classNamePos.w = classNameWidth;
		charLevel->setSize(charLevelPos);
		className->setSize(classNamePos);
		//messagePlayer(0, "%d | %d", charLevelPos.x - startX, 198 - (classNamePos.x + classNamePos.w));
	}
	if ( auto raceText = characterInnerFrame->findField("character race text") )
	{
		Monster type = stats[player.playernum]->type;
		std::string appearance = "";
		bool aestheticOnly = false;
		if ( player.entity )
		{
			if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
			{
				if ( stats[player.playernum]->appearance != 0 )
				{
					aestheticOnly = true;
					appearance = language[4068];
					type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
				}
			}
		}
		std::string race = getMonsterLocalizedName(type).c_str();
		capitalizeString(race);
		if ( type == HUMAN )
		{
			appearance = language[20 + stats[player.playernum]->appearance % NUMAPPEARANCES];
			capitalizeString(appearance);
		}
		bool centerIconAndText = false;
		if ( appearance != "" )
		{
			if ( aestheticOnly )
			{
				snprintf(buf, sizeof(buf), "%s %s", appearance.c_str(), race.c_str()); // 'guised skeleton'
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s %s", race.c_str(), appearance.c_str()); // 'human gloomforge'
			}
			centerIconAndText = true;
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s", race.c_str());
		}

		raceText->setText(buf);
		int width = 0;
		if ( auto textGet = Text::get(raceText->getText(), raceText->getFont(),
			raceText->getTextColor(), raceText->getOutlineColor()) )
		{
			width = textGet->getWidth();
		}

		if ( auto sexImg = characterInnerFrame->findImage("character sex img") )
		{
			if ( stats[player.playernum]->sex == sex_t::MALE )
			{
				if ( type == AUTOMATON )
				{
					sexImg->path = "images/ui/CharSheet/HUD_CharSheet_Sex_AutomatonM_02.png";
				}
				else
				{
					sexImg->path = "images/ui/CharSheet/HUD_CharSheet_Sex_M_02.png";
				}
			}
			else if ( stats[player.playernum]->sex == sex_t::FEMALE )
			{
				if ( type == AUTOMATON )
				{
					sexImg->path = "images/ui/CharSheet/HUD_CharSheet_Sex_AutomatonF_02.png";
				}
				else
				{
					sexImg->path = "images/ui/CharSheet/HUD_CharSheet_Sex_F_02.png";
				}
			}
			if ( auto imgGet = Image::get(sexImg->path.c_str()) )
			{
				sexImg->pos.w = imgGet->getWidth();
				sexImg->pos.h = imgGet->getHeight();
			}

			SDL_Rect raceTextPos = raceText->getSize();
			raceTextPos.x = 4;
			if ( centerIconAndText )
			{
				raceTextPos.x = 4 + ((sexImg->pos.w + 4)) / 2;
			}
			raceText->setSize(raceTextPos);

			sexImg->pos.x = 16;
			if ( centerIconAndText )
			{
				sexImg->pos.x = raceText->getSize().x + raceText->getSize().w / 2 - width / 2;
				sexImg->pos.x -= (sexImg->pos.w + 4);
			}
			sexImg->pos.y = raceText->getSize().y + raceText->getSize().h / 2 - sexImg->pos.h / 2;
		}
	}
	if ( auto floorFrame = sheetFrame->findFrame("dungeon floor frame") )
	{
		if ( auto floorLevelText = floorFrame->findField("dungeon level text") )
		{
			snprintf(buf, sizeof(buf), language[4052], currentlevel);
			floorLevelText->setText(buf);
		}
		if ( auto floorNameText = floorFrame->findField("dungeon name text") )
		{
			if ( mapDisplayNamesDescriptions.find(map.name) != mapDisplayNamesDescriptions.end() )
			{
				floorNameText->setText(mapDisplayNamesDescriptions[map.name].first.c_str());
				if ( selectedElement == SHEET_DUNGEON_FLOOR )
				{
					updateCharacterSheetTooltip(selectedElement, floorFrame->getSize());
				}
			}
			else
			{
				floorNameText->setText(map.name);
			}
		}
	}
	if ( auto gold = characterInnerFrame->findField("gold text") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->GOLD);
		gold->setText(buf);
		if ( selectedElement == SHEET_GOLD )
		{
			updateCharacterSheetTooltip(selectedElement, characterInfoFrame->getSize());
		}
	}
}

void Player::CharacterSheet_t::updateStats()
{
	auto statsFrame = sheetFrame->findFrame("stats");
	assert(statsFrame);

	auto statsPos = statsFrame->getSize();
	statsPos.x = sheetFrame->getSize().w - statsPos.w;
	statsFrame->setSize(statsPos);

	auto statsInnerFrame = statsFrame->findFrame("stats inner frame");
	assert(statsInnerFrame);

	const int rightAlignPosX = 20;
	const int leftAlignPosX = 30;
	auto statsInnerPos = statsInnerFrame->getSize();
	statsInnerPos.x = rightAlignPosX;
	if ( keystatus[SDL_SCANCODE_I] )
	{
		statsInnerPos.x = leftAlignPosX;
	}
	statsInnerFrame->setSize(statsInnerPos);

	char buf[32] = "";

	if ( auto field = statsInnerFrame->findField("str text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->STR);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("str text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			modifiedField->setText(buf);
			if ( modifiedStat > stats[player.playernum]->STR )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->STR )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
	}
	if ( auto field = statsInnerFrame->findField("dex text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->DEX);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("dex text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			modifiedField->setText(buf);
			if ( modifiedStat > stats[player.playernum]->DEX )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->DEX )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
	}
	if ( auto field = statsInnerFrame->findField("con text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->CON);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetCON(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("con text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			modifiedField->setText(buf);
			if ( modifiedStat > stats[player.playernum]->CON )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->CON )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
	}
	if ( auto field = statsInnerFrame->findField("int text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->INT);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetINT(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("int text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			modifiedField->setText(buf);
			if ( modifiedStat > stats[player.playernum]->INT )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->INT )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
	}
	if ( auto field = statsInnerFrame->findField("per text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->PER);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetPER(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("per text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			modifiedField->setText(buf);
			if ( modifiedStat > stats[player.playernum]->PER )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->PER )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
	}
	if ( auto field = statsInnerFrame->findField("chr text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->CHR);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetCHR(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("chr text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			modifiedField->setText(buf);
			if ( modifiedStat > stats[player.playernum]->CHR )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->CHR )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
	}
}

void Player::CharacterSheet_t::updateAttributes()
{
	auto attributesFrame = sheetFrame->findFrame("attributes");
	assert(attributesFrame);

	auto attributesPos = attributesFrame->getSize();
	attributesPos.x = sheetFrame->getSize().w - attributesPos.w;
	attributesFrame->setSize(attributesPos);

	auto attributesInnerFrame = attributesFrame->findFrame("attributes inner frame");
	assert(attributesInnerFrame);

	const int rightAlignPosX = 20;
	const int leftAlignPosX = 30;
	auto attributesInnerPos = attributesInnerFrame->getSize();
	attributesInnerPos.x = rightAlignPosX;
	/*if ( keystatus[SDL_SCANCODE_I] )
	{
		attributesInnerPos.x = leftAlignPosX;
	}*/
	attributesInnerFrame->setSize(attributesInnerPos);

	char buf[32] = "";

	if ( auto field = attributesInnerFrame->findField("atk text stat") )
	{
		Sint32 dummy[6];
		snprintf(buf, sizeof(buf), "%d", displayAttackPower(player.playernum, dummy));
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);
	}

	if ( auto field = attributesInnerFrame->findField("ac text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", AC(stats[player.playernum]));
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);
	}

	if ( auto field = attributesInnerFrame->findField("pwr text stat") )
	{
		real_t spellPower = (getBonusFromCasterOfSpellElement(player.entity, stats[player.playernum]) * 100.0) + 100.0;
		snprintf(buf, sizeof(buf), "%.f%%", spellPower);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);
	}

	if ( auto field = attributesInnerFrame->findField("res text stat") )
	{
		real_t resistance = 0.0;
		if ( players[player.playernum]->entity )
		{
			resistance = 100 - 100 / (players[player.playernum]->entity->getMagicResistance() + 1);
		}
		snprintf(buf, sizeof(buf), "%.f%%", resistance);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);
		if ( resistance > 0.01 )
		{
			field->setColor(hudColors.characterSheetGreen);
		}
	}

	if ( auto field = attributesInnerFrame->findField("regen text hp") )
	{
		real_t regen = 0.0;
		field->setColor(hudColors.characterSheetNeutral);
		if ( players[player.playernum]->entity && stats[player.playernum]->HP > 0 )
		{
			regen = (static_cast<real_t>(players[player.playernum]->entity->getHealthRegenInterval(*stats[player.playernum])) / TICKS_PER_SECOND);
			if ( stats[player.playernum]->type == SKELETON )
			{
				if ( !(svFlags & SV_FLAG_HUNGER) )
				{
					regen = HEAL_TIME * 4 / TICKS_PER_SECOND;
				}
			}
			if ( regen < 0 )
			{
				regen = 0.0;
				if ( !(svFlags & SV_FLAG_HUNGER) )
				{
					field->setColor(hudColors.characterSheetNeutral);
				}
				else
				{
					field->setColor(hudColors.characterSheetRed);
				}
			}
			else if ( regen < HEAL_TIME / TICKS_PER_SECOND )
			{
				field->setColor(hudColors.characterSheetGreen);
			}
		}
		else
		{
			regen = HEAL_TIME / TICKS_PER_SECOND;
		}

		if ( regen > 0.01 )
		{
			real_t nominalRegen = HEAL_TIME / TICKS_PER_SECOND;
			regen = nominalRegen / regen;
		}
		snprintf(buf, sizeof(buf), "%.f%%", regen * 100.0);
		field->setText(buf);
	}

	if ( auto field = attributesInnerFrame->findField("regen text mp") )
	{
		real_t regen = 0.0;
		field->setColor(hudColors.characterSheetNeutral);
		if ( players[player.playernum]->entity )
		{
			regen = (static_cast<real_t>(players[player.playernum]->entity->getManaRegenInterval(*stats[player.playernum])) / TICKS_PER_SECOND);
			if ( stats[player.playernum]->type == AUTOMATON )
			{
				if ( stats[player.playernum]->HUNGER <= 300 )
				{
					regen /= 6; // degrade faster
				}
				else if ( stats[player.playernum]->HUNGER > 1200 )
				{
					if ( stats[player.playernum]->MP / static_cast<real_t>(std::max(1, stats[player.playernum]->MAXMP)) <= 0.5 )
					{
						regen /= 4; // increase faster at < 50% mana
					}
					else
					{
						regen /= 2; // increase less faster at > 50% mana
					}
				}
				else if ( stats[player.playernum]->HUNGER > 300 )
				{
					// normal manaRegenInterval 300-1200 hunger.
				}
			}

			if ( regen < 0.0 /*stats[player]->playerRace == RACE_INSECTOID && stats[player]->appearance == 0*/ )
			{
				regen = 0.0;
			}

			if ( stats[player.playernum]->type == AUTOMATON )
			{
				if ( stats[player.playernum]->HUNGER <= 300 )
				{
					field->setColor(hudColors.characterSheetRed);
				}
				else if ( regen < static_cast<real_t>(players[player.playernum]->entity->getBaseManaRegen(*stats[player.playernum])) / TICKS_PER_SECOND )
				{
					field->setColor(hudColors.characterSheetGreen);
				}
			}
			else if ( stats[player.playernum]->playerRace == RACE_INSECTOID && stats[player.playernum]->appearance == 0 )
			{
				if ( !(svFlags & SV_FLAG_HUNGER) )
				{
					field->setColor(hudColors.characterSheetNeutral);
				}
				else
				{
					field->setColor(hudColors.characterSheetRed);
				}
			}
			else if ( regen < static_cast<real_t>(players[player.playernum]->entity->getBaseManaRegen(*stats[player.playernum])) / TICKS_PER_SECOND )
			{
				field->setColor(hudColors.characterSheetGreen);
			}
		}
		else
		{
			regen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
		}

		if ( regen > 0.01 )
		{
			real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
			regen = nominalRegen / regen;
		}
		snprintf(buf, sizeof(buf), "%.f%%", regen * 100.0);
		field->setText(buf);
	}

	if ( auto field = attributesInnerFrame->findField("weight text stat") )
	{
		Sint32 weight = 0;
		for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				weight += item->getWeight();
			}
		}
		weight += stats[player.playernum]->GOLD / 100;
		snprintf(buf, sizeof(buf), "%d", weight);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);
	}
}

void Player::Hotbar_t::processHotbar()
{
	char name[32];
	snprintf(name, sizeof(name), "player hotbar %d", player.playernum);
	if ( !hotbarFrame )
	{
		hotbarFrame = gui->addFrame(name);
		hotbarFrame->setHollow(true);
		hotbarFrame->setBorder(0);
		hotbarFrame->setOwner(player.playernum);
		createHotbar(player.playernum);
	}
	hotbarFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	if ( gamePaused || nohud || !players[player.playernum]->isLocalPlayer() )
	{
		// hide
		hotbarFrame->setDisabled(true);
	}
	else
	{
		hotbarFrame->setDisabled(false);
	}

	updateHotbar();
}

void createIngameHud(int player) {
    char name[32];
    snprintf(name, sizeof(name), "player hud %d", player);
    Frame* frame = gui->addFrame(name);

    players[player]->hud.hudFrame = frame;
    int playercount = 0;
    if (multiplayer == SINGLE) {
        for (int c = 0; c < MAXPLAYERS; ++c) {
            if (!client_disconnected[c]) {
                ++playercount;
            }
        }
    } else {
        playercount = 1;
    }

#ifdef NINTENDO
    static const char* bigfont = "rom://fonts/pixelmix.ttf#18";
    static const char* smallfont = "rom://fonts/pixel_maz.ttf#32";
#else
    static const char* bigfont = "fonts/pixelmix.ttf#18";
    static const char* smallfont = "fonts/pixel_maz.ttf#14";
#endif // NINTENDO

    // big empty frame to serve as the root
    if (playercount == 1) {
        frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
    } else if (playercount == 2) {
        int y = (player % 2) * Frame::virtualScreenY / 2;
        frame->setSize(SDL_Rect{0, y, Frame::virtualScreenX, Frame::virtualScreenY / 2});
    } else if (playercount >= 3 || playercount <= 4) {
        int x = (player % 2) * Frame::virtualScreenX / 2;
        int y = (player / 2) * Frame::virtualScreenY / 2;
        frame->setSize(SDL_Rect{x, y, Frame::virtualScreenX / 2, Frame::virtualScreenY / 2});
    }
    frame->setHollow(true);
    frame->setBorder(0);

    // chat
    Frame* chat = frame->addFrame("chat");
    chat->setSize(SDL_Rect{224, 16, 832, 200});
    chat->setFont(bigfont);
    {
        auto e = chat->addEntry("chat", true);
        e->color = 0xff00ffff;
        e->text = "Player 3: Chatlog text is so great what do you think?";
    }
    {
        auto e = chat->addEntry("message", true);
        e->color = 0xffffffff;
        e->text = "You have been hit by a dog.";
    }
    {
        auto e = chat->addEntry("message", true);
        e->color = 0xffffffff;
        e->text = "Yowza, dog hits smart!";
    }
    {
        auto e = chat->addEntry("message", true);
        e->color = 0xffffffff;
        e->text = "The dog hit smarts.";
    }
    {
        auto e = chat->addEntry("chat", true);
        e->color = 0xffff8800;
        e->text = "Player 2: Chatlog text is the best, I swear my life on it.";
    }

    // other players' statuses
    for (int i = 0, c = 0; c < MAXPLAYERS; ++c) {
        if (c == player) {
            continue;
        }

        // player name
        {
            char name[32];
            snprintf(name, sizeof(name), "Player %d Name", c + 1);
            Field* field = frame->addField(name, sizeof(name));
            Uint32 color = 0xffffffff;
            switch (c) {
            case 0: color = 0xffffffff; break;
            case 1: color = 0xffff8800; break;
            case 2: color = 0xff00ffff; break;
            case 3: color = 0xffff00ff; break;
            }
            field->setSize(SDL_Rect{16, 8 + 56 * i, 320, 64});
            field->setText(name);
            field->setFont(smallfont);
            field->setColor(color);
        }

        // hp bar
        {
            char name[32];
            snprintf(name, sizeof(name), "hp %d", c + 1);
            Frame* hp = frame->addFrame(name);
            hp->setColor(0xffffffff);
            hp->setBorderStyle(Frame::BORDER_BEVEL_HIGH);
            hp->setBorder(2);
            hp->setSize(SDL_Rect{16, 32 + 56 * i, 160, 16});
            auto red = hp->addImage(
                SDL_Rect{4, 4, hp->getSize().w - 8, hp->getSize().h - 8},
                0xff8888ff,
                "images/system/white.png",
                "red"
            );
        }

        // mp bar
        {
            char name[32];
            snprintf(name, sizeof(name), "mp %d", c + 1);
            Frame* mp = frame->addFrame(name);
            mp->setColor(0xffffffff);
            mp->setBorderStyle(Frame::BORDER_BEVEL_HIGH);
            mp->setBorder(2);
            mp->setSize(SDL_Rect{16, 48 + 56 * i, 160, 16});
            auto blue = mp->addImage(
                SDL_Rect{4, 4, mp->getSize().w - 8, mp->getSize().h - 8},
                0xffff8888,
                "images/system/white.png",
                "blue"
            );
        }

        // position
        ++i;
    }

    // ally hp
    for (int c = 0; c < 8; ++c) {
        // text
        {
            char text[32];
            snprintf(text, sizeof(text), "Ally %d", c + 1);
            Field* field = frame->addField(text, sizeof(text));
            field->setText(text);
            field->setSize(SDL_Rect{frame->getSize().w - 176, 8 + 18 * c, 64, 24});
            field->setFont(smallfont);
            field->setColor(0xff888888);
        }

        // bar
        {
            char name[32];
            snprintf(name, sizeof(name), "ally hp bar %d", c + 1);
            Frame* bar = frame->addFrame(name);
            bar->setSize(SDL_Rect{frame->getSize().w - 128, 14 + 18 * c, 112, 14});
            bar->setColor(0xffaaaaaa);
            auto red = bar->addImage(
                SDL_Rect{2, 2, bar->getSize().w - 4, bar->getSize().h - 4},
                0xff4444dd,
                "images/system/white.png",
                "red"
            );
        }
    }

    // hotbar
    static int num_hotbar_slots = 10;
    for (int c = 0; c < num_hotbar_slots; ++c) {
        Image* img = Image::get("images/system/hotbar_slot.png");
        Uint32 color = 0xffffffff;
        if (c == 2) {
            // selected one is highlighted yellow
            color = 0xff00ffff;
        }
        if (img) {
            int w = img->getWidth();
            int h = img->getHeight();
            int x = frame->getSize().w / 2 - w * num_hotbar_slots / 2 + w * c;
            int y = frame->getSize().h - h - 32;
            frame->addImage(
                SDL_Rect{x, y, w, h},
                color,
                "images/system/hotbar_slot.png",
                "hotbarslot"
            );
        }
    }

    // selected spell
    Image* spellbox_img = Image::get("images/system/hotbar_slot.png");
    if (spellbox_img) {
        int w = spellbox_img->getWidth();
        int h = spellbox_img->getHeight();
        int x = frame->getSize().w / 2 + w * num_hotbar_slots / 2 + 16;
        int y = frame->getSize().h - h - 32;
        frame->addImage(
            SDL_Rect{x, y, w, h},
            0xffffffff,
            "images/system/hotbar_slot.png",
            "spellbox"
        );
    }

    // xp bar
    if (spellbox_img) {
        int w = spellbox_img->getWidth();
        int h = spellbox_img->getHeight();

        Frame* xpbar = frame->addFrame("xpbar");
        xpbar->setSize(SDL_Rect{
            frame->getSize().w / 2 - w * num_hotbar_slots / 2,
            frame->getSize().h - 32,
            w * num_hotbar_slots,
            24
            }
        );
        xpbar->setColor(0xffaaaaaa);
        {
            auto progress = xpbar->addImage(
                SDL_Rect{2, 2, (xpbar->getSize().w - 4) / 4, xpbar->getSize().h - 4},
                0xffffffff,
                "images/system/white.png"
            );

            Field* text = xpbar->addField("text", 64);
            text->setSize(SDL_Rect{2, 4, xpbar->getSize().w - 4, xpbar->getSize().h - 4});
            text->setText("22 / 100 XP");
            text->setFont(smallfont);
            text->setColor(0xff888888);
            text->setHJustify(Field::justify_t::CENTER);
            text->setVJustify(Field::justify_t::CENTER);
        }
    }

    // hunger icon
    auto hunger = frame->addImage(
        SDL_Rect{16, frame->getSize().h - 160, 64, 64},
        0xffffffff,
        "images/system/Hunger.png",
        "hunger"
    );

    // hp
    {
        Frame* hp = frame->addFrame("hp");
        hp->setColor(0xffffffff);
        hp->setSize(SDL_Rect{16, frame->getSize().h - 96, 40, 24});
        {
            auto red = hp->addImage(
                SDL_Rect{4, 4, 32, 16},
                0xff8888ff,
                "images/system/white.png",
                "red"
            );
        }
        Frame* hp_bar = frame->addFrame("hp_bar");
        hp_bar->setColor(0xffffffff);
        hp_bar->setSize(SDL_Rect{56, frame->getSize().h - 96, 256, 24});
        {
            auto red = hp_bar->addImage(
                SDL_Rect{4, 4, hp_bar->getSize().w - 8, 16},
                0xff8888ff,
                "images/system/white.png",
                "red"
            );
            Field* text = hp_bar->addField("text", 64);
            text->setSize(SDL_Rect{6, 3, 64, hp_bar->getSize().h - 4});
            text->setText("HP");
            text->setFont(smallfont);
            text->setColor(0xffffffff);
            text->setHJustify(Field::justify_t::LEFT);
            text->setVJustify(Field::justify_t::CENTER);
        }
    }

    // mp
    {
        Frame* mp = frame->addFrame("mp");
        mp->setColor(0xffffffff);
        mp->setSize(SDL_Rect{16, frame->getSize().h - 68, 40, 24});
        {
            auto blue = mp->addImage(
                SDL_Rect{4, 4, 32, 16},
                0xffff8888,
                "images/system/white.png",
                "blue"
            );
        }
        Frame* mp_bar = frame->addFrame("mp_bar");
        mp_bar->setColor(0xffffffff);
        mp_bar->setSize(SDL_Rect{56, frame->getSize().h - 68, 64, 24});
        {
            auto blue = mp_bar->addImage(
                SDL_Rect{4, 4, mp_bar->getSize().w - 8, 16},
                0xffff8888,
                "images/system/white.png",
                "blue"
            );
            Field* text = mp_bar->addField("text", 64);
            text->setSize(SDL_Rect{6, 3, 64, mp_bar->getSize().h - 4});
            text->setText("MP");
            text->setFont(smallfont);
            text->setColor(0xffffffff);
            text->setHJustify(Field::justify_t::LEFT);
            text->setVJustify(Field::justify_t::CENTER);
        }
    }

    // status effect
    auto effect = frame->addImage(
        SDL_Rect{16, frame->getSize().h - 40, 32, 32},
        0xffffffff,
        "images/system/drunk.png",
        "status"
    );
}

void newIngameHud() {
    if (!nohud) {
        // here is where splitscreen
        if (!players[clientnum]->hud.hudFrame) {
            createIngameHud(clientnum);
        }

        // original minimap already works fine, so just reuse it
        if (multiplayer == SINGLE) {
            for (int c = 0; c < MAXPLAYERS; ++c) {
                if (!client_disconnected[c]) {
                    drawMinimap(c);
                }
            }
        } else {
            drawMinimap(0);
        }
    }
}

void createPlayerInventorySlotFrameElements(Frame* slotFrame)
{
	const SDL_Rect slotSize = SDL_Rect{ 0, 0, slotFrame->getSize().w, slotFrame->getSize().h };
	SDL_Rect coloredBackgroundPos = SDL_Rect{ slotSize.x + 2, slotSize.y + 2, slotSize.w - 2, slotSize.h - 2 };

	auto beatitudeFrame = slotFrame->addFrame("beatitude status frame"); // covers unidentified status as well
	beatitudeFrame->setSize(slotSize);
	beatitudeFrame->setHollow(true);
	beatitudeFrame->setDisabled(true);
	beatitudeFrame->addImage(coloredBackgroundPos, 0xFFFFFFFF, "images/system/white.png", "beatitude status bg");

	auto brokenStatusFrame = slotFrame->addFrame("broken status frame");
	brokenStatusFrame->setSize(slotSize);
	brokenStatusFrame->setHollow(true);
	brokenStatusFrame->setDisabled(true);
	brokenStatusFrame->addImage(coloredBackgroundPos, SDL_MapRGBA(mainsurface->format, 160, 160, 160, 64), "images/system/white.png", "broken status bg");

	auto itemSpriteFrame = slotFrame->addFrame("item sprite frame");

	// cut off the slot 2px borders
	SDL_Rect itemSpriteBorder = { slotSize.x + 2, slotSize.y + 2, slotFrame->getSize().w - 2, slotFrame->getSize().h - 2 };
	const int itemSpriteSize = players[slotFrame->getOwner()]->inventoryUI.getItemSpriteSize();
	const int alignOffset = (itemSpriteBorder.w - itemSpriteSize) / 2; // align the item sprite within the box by this offset to center
	itemSpriteBorder.x += alignOffset;
	itemSpriteBorder.y += alignOffset;
	itemSpriteBorder.w = itemSpriteSize;
	itemSpriteBorder.h = itemSpriteSize;

	itemSpriteFrame->setSize(SDL_Rect{ itemSpriteBorder.x, itemSpriteBorder.y, 
		itemSpriteBorder.w, itemSpriteBorder.h });
	itemSpriteFrame->setHollow(true);
	itemSpriteFrame->setDisabled(true);
	SDL_Rect imgPos{ 0, 0, itemSpriteFrame->getSize().w, itemSpriteFrame->getSize().h };
	itemSpriteFrame->addImage(imgPos, 0xFFFFFFFF, "images/system/white.png", "item sprite img");

	auto unusableFrame = slotFrame->addFrame("unusable item frame");
	unusableFrame->setSize(slotSize);
	unusableFrame->setHollow(true);
	unusableFrame->setDisabled(true);
	unusableFrame->addImage(coloredBackgroundPos, SDL_MapRGBA(mainsurface->format, 64, 64, 64, 144), "images/system/white.png", "unusable item bg");


	static const char* qtyfont = "fonts/pixel_maz.ttf#32#2";
	auto quantityFrame = slotFrame->addFrame("quantity frame");
	quantityFrame->setSize(slotSize);
	quantityFrame->setHollow(true);
	Field* qtyText = quantityFrame->addField("quantity text", 32);
	qtyText->setFont(qtyfont);
	qtyText->setColor(0xffffffff);
	qtyText->setHJustify(Field::justify_t::BOTTOM);
	qtyText->setVJustify(Field::justify_t::RIGHT);
	qtyText->setText("10");
	qtyText->setSize(SDL_Rect{ 0, 6, quantityFrame->getSize().w, quantityFrame->getSize().h });

	auto equippedIconFrame = slotFrame->addFrame("equipped icon frame");
	equippedIconFrame->setSize(slotSize);
	equippedIconFrame->setHollow(true);
	SDL_Rect equippedImgPos = { 3, slotSize.h - 17, 16, 16 };
	equippedIconFrame->addImage(equippedImgPos, 0xFFFFFFFF, "images/system/Equipped.png", "equipped icon img");

	auto brokenIconFrame = slotFrame->addFrame("broken icon frame");
	brokenIconFrame->setSize(slotSize);
	brokenIconFrame->setHollow(true);
	brokenIconFrame->addImage(equippedImgPos, 0xFFFFFFFF, "images/system/Broken.png", "broken icon img");
}

void resetInventorySlotFrames(const int player)
{
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	if ( Frame* inventoryFrame = gui->findFrame(name) )
	{
		for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
		{
			for ( int y = Player::Inventory_t::PaperDollRows::DOLL_ROW_1; y < players[player]->inventoryUI.getSizeY(); ++y )
			{
				if ( auto slotFrame = players[player]->inventoryUI.getInventorySlotFrame(x, y) )
				{
					slotFrame->setDisabled(true);
				}
			}
		}
	}
}

bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy)
{
	if ( !gui )
	{
		return false;
	}
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	if ( Frame* inventoryFrame = gui->findFrame(name) )
	{
		for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
		{
			for ( int y = Player::Inventory_t::DOLL_ROW_1; y < players[player]->inventoryUI.getSizeY(); ++y )
			{
				auto slotFrame = players[player]->inventoryUI.getInventorySlotFrame(x, y);
				if ( !slotFrame )
				{
					continue;
				}

				if ( slotFrame->capturesMouseInRealtimeCoords() )
				{
					outx = x;
					outy = y;
					return true;
				}
			}
		}
	}
	return false;
}

void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr)
{
	if ( !itemPtr || !slotFrame )
	{
		return;
	}

	Item* item = (Item*)itemPtr;

	int player = slotFrame->getOwner();

	slotFrame->setDisabled(false);

	auto spriteImageFrame = slotFrame->findFrame("item sprite frame");
	auto spriteImage = spriteImageFrame->findImage("item sprite img");

	spriteImage->path = getItemSpritePath(player, *item);
	bool disableBackgrounds = false;
	if ( spriteImage->path != "" )
	{
		spriteImageFrame->setDisabled(false);
		if ( inputs.getUIInteraction(player)->selectedItem == item )
		{
			if ( !strcmp(slotFrame->getName(), "hotbar slot item") ) // hotbar slots
			{
				// fade this icon
				spriteImage->color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 128);
				disableBackgrounds = true;
			}
		}
		else if ( !strcmp(slotFrame->getName(), "hotbar slot item") ) // hotbar slots
		{
			auto& hotbar_t = players[player]->hotbar;
			bool tryDimHotbarSlot = false;
			if ( hotbar_t.useHotbarFaceMenu && hotbar_t.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE )
			{
				tryDimHotbarSlot = true;
			}
			spriteImage->color = 0xFFFFFFFF;
			if ( tryDimHotbarSlot )
			{
				std::string hotbarSlotParentStr = slotFrame->getParent()->getName();
				if ( hotbarSlotParentStr.find("hotbar slot ") != std::string::npos )
				{
					int num = stoi(hotbarSlotParentStr.substr(strlen("hotbar slot ")));
					if ( hotbar_t.faceMenuButtonHeld != hotbar_t.getFaceMenuGroupForSlot(num) )
					{
						// fade this icon
						spriteImage->color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 128);
					}
				}
			}
		}
		else
		{
			spriteImage->color = 0xFFFFFFFF;
		}
	}

	if ( auto qtyFrame = slotFrame->findFrame("quantity frame") )
	{
		qtyFrame->setDisabled(true);
		if ( item->count > 1 )
		{
			qtyFrame->setDisabled(false);
			if ( auto qtyText = qtyFrame->findField("quantity text") )
			{
				char qtybuf[32] = "";
				snprintf(qtybuf, sizeof(qtybuf), "%d", item->count);
				qtyText->setText(qtybuf);
			}
		}
	}

	if ( !strcmp(slotFrame->getName(), "dragging inventory item") ) // dragging item, no need for colors
	{
		disableBackgrounds = true;
	}
	
	if ( auto beatitudeFrame = slotFrame->findFrame("beatitude status frame") )
	{
		beatitudeFrame->setDisabled(true);
		if ( !disableBackgrounds )
		{
			if ( auto beatitudeImg = beatitudeFrame->findImage("beatitude status bg") )
			{
				if ( !item->identified )
				{
					beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 128, 128, 0, 125);
					beatitudeFrame->setDisabled(false);
				}
				else if ( item->beatitude < 0 )
				{
					beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 128, 0, 0, 125);
					beatitudeFrame->setDisabled(false);
				}
				else if ( item->beatitude > 0 )
				{
					if ( colorblind )
					{
						beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 100, 245, 255, 65);
					}
					else
					{
						beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 0, 255, 0, 65);
					}
					beatitudeFrame->setDisabled(false);
				}
			}
		}
	}

	if ( auto brokenStatusFrame = slotFrame->findFrame("broken status frame") )
	{
		brokenStatusFrame->setDisabled(true);
		if ( !disableBackgrounds )
		{
			if ( item->status == BROKEN )
			{
				brokenStatusFrame->setDisabled(false);
			}
		}
	}

	if ( auto unusableFrame = slotFrame->findFrame("unusable item frame") )
	{
		bool greyedOut = false;
		unusableFrame->setDisabled(true);

		if ( !disableBackgrounds )
		{
			if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
			{
				// shape shifted, disable some items
				if ( !item->usableWhileShapeshifted(stats[player]) )
				{
					greyedOut = true;
				}
			}
			if ( !greyedOut && client_classes[player] == CLASS_SHAMAN
				&& item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
			{
				greyedOut = true;
			}

			if ( greyedOut )
			{
				unusableFrame->setDisabled(false);
			}
		}
	}

	bool equipped = false;
	bool broken = false;
	if ( itemCategory(item) != SPELL_CAT )
	{
		if ( itemIsEquipped(item, player) )
		{
			equipped = true;
		}
		else if ( item->status == BROKEN )
		{
			broken = true;
		}
	}
	else
	{
		spell_t* spell = getSpellFromItem(player, item);
		if ( players[player]->magic.selectedSpell() == spell
			&& (players[player]->magic.selected_spell_last_appearance == item->appearance || players[player]->magic.selected_spell_last_appearance == -1) )
		{
			equipped = true;
		}
	}

	if ( auto equippedIconFrame = slotFrame->findFrame("equipped icon frame") )
	{
		equippedIconFrame->setDisabled(true);
		if ( equipped && !disableBackgrounds )
		{
			equippedIconFrame->setDisabled(false);
		}
	}
	if ( auto brokenIconFrame = slotFrame->findFrame("broken icon frame") )
	{
		brokenIconFrame->setDisabled(true);
		if ( broken && !disableBackgrounds )
		{
			brokenIconFrame->setDisabled(false);
		}
	}
}

void createInventoryTooltipFrame(const int player)
{
	if ( !gui )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player tooltip %d", player);

	if ( !players[player]->inventoryUI.tooltipFrame )
	{
		players[player]->inventoryUI.tooltipFrame = gui->addFrame(name);
		auto tooltipFrame = players[player]->inventoryUI.tooltipFrame;
		tooltipFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipFrame->setDisabled(true);
		tooltipFrame->setInheritParentFrameOpacity(false);
	}
	else
	{
		return;
	}

	auto tooltipFrame = players[player]->inventoryUI.tooltipFrame;

	Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255);
	tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 28 },
		color, "images/ui/Inventory/tooltips/Hover_T00.png", "tooltip top background");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 28 },
		color, "images/ui/Inventory/tooltips/Hover_TL00.png", "tooltip top left");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 28 },
		color, "images/ui/Inventory/tooltips/Hover_TR00.png", "tooltip top right");

	tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 52 },
		color, "images/ui/Inventory/tooltips/Hover_C00.png", "tooltip middle background");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 52 },
		color, "images/ui/Inventory/tooltips/Hover_L00.png", "tooltip middle left");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 52 },
		color, "images/ui/Inventory/tooltips/Hover_R00.png", "tooltip middle right");

	tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 26 },
		color, "images/ui/Inventory/tooltips/Hover_B00.png", "tooltip bottom background");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 26 },
		color, "images/ui/Inventory/tooltips/Hover_BL01.png", "tooltip bottom left");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 26 },
		color, "images/ui/Inventory/tooltips/Hover_BR01.png", "tooltip bottom right");

	const std::string headerFont = "fonts/pixelmix.ttf#14#2";
	const std::string bodyFont = "fonts/pixelmix.ttf#12#2";
	//const std::string headerFont = "fonts/pixelmix.ttf#16#2";
	//const std::string bodyFont = "fonts/pixel_maz_multiline.ttf#16#1";

	auto tooltipTextField = tooltipFrame->addField("inventory mouse tooltip header", 1024);
	tooltipTextField->setText("Nothing");
	tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
	tooltipTextField->setFont(headerFont.c_str());
	tooltipTextField->setHJustify(Field::justify_t::LEFT);
	tooltipTextField->setVJustify(Field::justify_t::CENTER);
	tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 67, 195, 157, 255));

	// temporary debug stuff
	{
		Frame::image_t* tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip min");
		tmp->color = SDL_MapRGBA(mainsurface->format, 255, 0, 0, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip max");
		tmp->color = SDL_MapRGBA(mainsurface->format, 0, 255, 0, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip header max");
		tmp->color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip header bg");
		tmp->color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip header bg new");
		tmp->color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
		tmp->disabled = true;
	}

	if ( auto attrFrame = tooltipFrame->addFrame("inventory mouse tooltip attributes frame") )
	{
		attrFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		auto spellImageBg = attrFrame->addImage(SDL_Rect{ 0, 0, 52, 52 },
			0xFFFFFFFF, "images/ui/Inventory/tooltips/SpellBorder_00.png", "inventory mouse tooltip spell image bg");
		spellImageBg->disabled = true;
		//spellImageBg->color = SDL_MapRGBA(mainsurface->format, 125, 125, 125, 228);
		auto spellImage = attrFrame->addImage(SDL_Rect{ 0, 0, 40, 40 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip spell image");
		spellImage->disabled = true;

		attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			0xFFFFFFFF, "images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Damage_00.png", "inventory mouse tooltip primary image");
		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value highlight", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value positive text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value negative text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value slot name", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::RIGHT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			0xFFFFFFFF, "images/system/con32.png", "inventory mouse tooltip secondary image");
		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value highlight", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value positive text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value negative text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			0xFFFFFFFF, "images/system/con32.png", "inventory mouse tooltip third image");
		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value highlight", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value positive text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value negative text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip attributes text", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));
	}
	if ( auto descFrame = tooltipFrame->addFrame("inventory mouse tooltip description frame") )
	{
		descFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		descFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
			SDL_MapRGBA(mainsurface->format, 49, 53, 61, 255),
			"images/system/white.png", "inventory mouse tooltip description divider");

		tooltipTextField = descFrame->addField("inventory mouse tooltip description", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 67, 195, 157, 255));

		tooltipTextField = descFrame->addField("inventory mouse tooltip description positive text", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		//tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 1, 151, 246, 255));
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		tooltipTextField = descFrame->addField("inventory mouse tooltip description negative text", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 215, 38, 61, 255));
	}
	if ( auto valueFrame = tooltipFrame->addFrame("inventory mouse tooltip value frame") )
	{
		valueFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		valueFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			SDL_MapRGBA(mainsurface->format, 49, 53, 61, 255), 
			"images/system/white.png", "inventory mouse tooltip value background");

		valueFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
			SDL_MapRGBA(mainsurface->format, 49, 53, 61, 255),
			"images/system/white.png", "inventory mouse tooltip value divider");

		tooltipTextField = valueFrame->addField("inventory mouse tooltip identified value", 64);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		valueFrame->addImage(SDL_Rect{ 0, 0, 16, 16 },
			0xFFFFFFFF, 
			"images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Money_00.png",
			"inventory mouse tooltip gold image");

		tooltipTextField = valueFrame->addField("inventory mouse tooltip gold value", 64);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));

		valueFrame->addImage(SDL_Rect{ 0, 0, 16, 16 },
			0xFFFFFFFF, 
			"images/ui/Inventory/tooltips/HUD_Tooltip_Icon_WGT_00.png", 
			"inventory mouse tooltip weight image");

		tooltipTextField = valueFrame->addField("inventory mouse tooltip weight value", 64);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255));
	}
	if ( auto promptFrame = tooltipFrame->addFrame("inventory mouse tooltip prompt frame") )
	{
		promptFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		tooltipTextField = promptFrame->addField("inventory mouse tooltip prompt", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::RIGHT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(SDL_MapRGBA(mainsurface->format, 148, 82, 3, 255));
	}

	snprintf(name, sizeof(name), "player interact %d", player);
	if ( auto interactFrame = gui->addFrame(name) )
	{
		players[player]->inventoryUI.interactFrame = interactFrame;
		const int interactWidth = 106;
		interactFrame->setSize(SDL_Rect{ 0, 0, interactWidth + 6 * 2, 100 });
		interactFrame->setDisabled(true);
		interactFrame->setInheritParentFrameOpacity(false);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255);
		const int topBackgroundHeight = 30;
		const int optionHeight = 20;

		interactFrame->addImage(SDL_Rect{ 12, 0, 0, 34 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_T01.png", "interact top background");
		interactFrame->addImage(SDL_Rect{ 0, 0, 12, 34 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_TL01.png", "interact top left");
		interactFrame->addImage(SDL_Rect{ 0, 0, 12, 34 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_TR01.png", "interact top right");

		interactFrame->addImage(SDL_Rect{ 4, 34, 0, 76 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_C01.png", "interact middle background");
		interactFrame->addImage(SDL_Rect{ 0, 34, 4, 76 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_L01.png", "interact middle left");
		interactFrame->addImage(SDL_Rect{ 0, 34, 4, 76 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_R01.png", "interact middle right");

		interactFrame->addImage(SDL_Rect{ 12, 96, 0, 10 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_B01.png", "interact bottom background");
		interactFrame->addImage(SDL_Rect{ 0, 96, 12, 10 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_BL01.png", "interact bottom left");
		interactFrame->addImage(SDL_Rect{ 0, 96, 12, 10 },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_BR01.png", "interact bottom right");

		/*interactFrame->addImage(SDL_Rect{ 16, 0, interactWidth, topBackgroundHeight },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_T00.png", "interact top background");
		interactFrame->addImage(SDL_Rect{ 0, 0, 16, topBackgroundHeight },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_TL00.png", "interact top left");
		interactFrame->addImage(SDL_Rect{ interactWidth + 16, 0, 16, topBackgroundHeight },
			color, "images/ui/Inventory/tooltips/HoverItemMenu_TR00.png", "interact top right");

		interactFrame->addImage(SDL_Rect{ 0, topBackgroundHeight, 6, 76 },
			color, "images/ui/Inventory/tooltips/HoverExt_L00.png", "interact middle left");
		interactFrame->addImage(SDL_Rect{ interactWidth + 6, topBackgroundHeight, 6, 76 },
			color, "images/ui/Inventory/tooltips/HoverExt_R00.png", "interact middle right");
		interactFrame->addImage(SDL_Rect{ 4, topBackgroundHeight, interactWidth + 2, 76 },
			hudColors.itemContextMenuOptionImg, "images/system/white.png", "interact middle background");

		interactFrame->addImage(SDL_Rect{ 4, 96, interactWidth + 4, 4 },
			color, "images/ui/Inventory/tooltips/HoverExt_B00.png", "interact bottom background");
		interactFrame->addImage(SDL_Rect{ 0, 96, 4, 4 },
			color, "images/ui/Inventory/tooltips/HoverExt_BL00.png", "interact bottom left");
		interactFrame->addImage(SDL_Rect{ interactWidth + 4 * 2, 96, 4, 4 },
			color, "images/ui/Inventory/tooltips/HoverExt_BR00.png", "interact bottom right");*/

		interactFrame->addImage(SDL_Rect{ 6, optionHeight, interactWidth, 76 },
			hudColors.itemContextMenuOptionSelectedImg, "images/system/whitecurve.png", "interact selected highlight");

		const char* interactFont = "fonts/pixel_maz.ttf#32#2";

		auto interactText = interactFrame->addField("interact text", 32);
		interactText->setText(language[4040]);
		interactText->setSize(SDL_Rect{ 0, 2, 0, topBackgroundHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::CENTER);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(hudColors.itemContextMenuHeadingText);

		const int interactOptionStartX = 4;
		const int interactOptionStartY = 36;
		const int glyphSize = 20;

		auto interactGlyph1 = interactFrame->addImage(
			SDL_Rect{ interactOptionStartX + 4, interactOptionStartY + 4, glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 1");

		const int textAlignX = interactGlyph1->pos.x + interactGlyph1->pos.w + 6;
		int textAlignY = interactGlyph1->pos.y - 8;
		const int textWidth = 80;
		const int textHeight = glyphSize + 8;

		Uint32 textColor = hudColors.itemContextMenuOptionText;
		
		interactText = interactFrame->addField("interact option 1", 32);
		interactText->setText("");
		interactText->setSize(SDL_Rect{ 
			textAlignX,
			textAlignY,
			textWidth, textHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::LEFT);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(textColor);

		auto interactGlyph2 = interactFrame->addImage(
			SDL_Rect{  interactOptionStartX + 4, 
				interactGlyph1->pos.y + interactGlyph1->pos.h + 4, 
				glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 2");

		textAlignY = interactGlyph2->pos.y - 8;
		interactText = interactFrame->addField("interact option 2", 32);
		interactText->setText("");
		interactText->setSize(SDL_Rect{
			textAlignX,
			textAlignY,
			textWidth, textHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::LEFT);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(textColor);

		auto interactGlyph3 = interactFrame->addImage(
			SDL_Rect{ interactOptionStartX + 4,
			interactGlyph2->pos.y + interactGlyph2->pos.h + 4,
			glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 3");

		textAlignY = interactGlyph3->pos.y - 8;
		interactText = interactFrame->addField("interact option 3", 32);
		interactText->setText("");
		interactText->setSize(SDL_Rect{
			textAlignX,
			textAlignY,
			textWidth, textHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::LEFT);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(textColor);

		auto interactGlyph4 = interactFrame->addImage(
			SDL_Rect{ interactOptionStartX + 4,
			interactGlyph3->pos.y + interactGlyph3->pos.h + 4,
			glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 4");

		textAlignY = interactGlyph4->pos.y - 8;
		interactText = interactFrame->addField("interact option 4", 32);
		interactText->setText("");
		interactText->setSize(SDL_Rect{
			textAlignX,
			textAlignY,
			textWidth, textHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::LEFT);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(textColor);

		auto interactGlyph5 = interactFrame->addImage(
			SDL_Rect{ interactOptionStartX + 4,
			interactGlyph4->pos.y + interactGlyph4->pos.h + 4,
			glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 5");

		textAlignY = interactGlyph5->pos.y - 8;
		interactText = interactFrame->addField("interact option 5", 32);
		interactText->setText("");
		interactText->setSize(SDL_Rect{
			textAlignX,
			textAlignY,
			textWidth, textHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::LEFT);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(textColor);
	}

	snprintf(name, sizeof(name), "player item prompt %d", player);
	if ( auto promptFrame = gui->addFrame(name) )
	{
		players[player]->inventoryUI.tooltipPromptFrame = promptFrame;
		const int interactWidth = 0;
		SDL_Rect promptSize{ 0, 0, interactWidth + 6 * 2, 100 };
		promptFrame->setDisabled(true);
		promptFrame->setInheritParentFrameOpacity(false);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 64);

		auto middleCenter = promptFrame->addImage(SDL_Rect{ 6, 2, interactWidth, 76 },
			color, "images/ui/Inventory/tooltips/Hover_C00.png", "interact middle background");
		auto middleTop = promptFrame->addImage(SDL_Rect{ 6, 0, interactWidth, 2 },
			color, "images/ui/Inventory/tooltips/HoverExt_C00.png", "interact middle top background");
		auto middleBottom = promptFrame->addImage(SDL_Rect{ 6, 0, interactWidth, 2 },
			color, "images/ui/Inventory/tooltips/HoverExt_C00.png", "interact middle bottom background");
		auto middleLeft = promptFrame->addImage(SDL_Rect{ 0, 0, 6, 76 },
			color, "images/ui/Inventory/tooltips/HoverExt_L00.png", "interact middle left");
		auto middleRight = promptFrame->addImage(SDL_Rect{ interactWidth + 6, 0, 6, 76 },
			color, "images/ui/Inventory/tooltips/HoverExt_R00.png", "interact middle right");

		auto bottomCenter = promptFrame->addImage(SDL_Rect{ 4, 76, interactWidth + 4, 4 },
			color, "images/ui/Inventory/tooltips/HoverExt_B00.png", "interact bottom background");
		auto bottomLeft = promptFrame->addImage(SDL_Rect{ 0, 76, 4, 4 },
			color, "images/ui/Inventory/tooltips/HoverExt_BL00.png", "interact bottom left");
		auto bottomRight = promptFrame->addImage(SDL_Rect{ interactWidth + 4 * 2, 76, 4, 4 },
			color, "images/ui/Inventory/tooltips/HoverExt_BR00.png", "interact bottom right");

		const int interactOptionStartX = 60;
		const int interactOptionStartY = 8;
		const int glyphSize = 20;

		auto interactGlyph1 = promptFrame->addImage(
			SDL_Rect{ interactOptionStartX, interactOptionStartY, glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 1");

		auto interactGlyph2 = promptFrame->addImage(
			SDL_Rect{ interactGlyph1->pos.x - (glyphSize / 2), 
			interactGlyph1->pos.y + glyphSize,
			glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 2");

		auto interactGlyph3 = promptFrame->addImage(
			SDL_Rect{ interactGlyph1->pos.x + (glyphSize + glyphSize / 4), interactGlyph1->pos.y, glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 3");

		auto interactGlyph4 = promptFrame->addImage(
			SDL_Rect{ interactGlyph3->pos.x - (glyphSize / 2), interactGlyph2->pos.y,
			glyphSize, glyphSize },
			0xFFFFFFFF, "", "glyph 4");

		const int textWidth = 80;
		const int textHeight = glyphSize + 8;

		Uint32 promptTextColor = SDL_MapRGBA(mainsurface->format, 188, 154, 114, 255);
		const char * promptFont = "fonts/pixel_maz.ttf#32#2";
		int textAlignY = interactGlyph1->pos.y - 4;
		int textAlignXRightJustify = interactGlyph1->pos.x - 6 - textWidth;
		auto promptText = promptFrame->addField("txt 1", 32);
		promptText->setText(language[4050]);
		promptText->setSize(SDL_Rect{
			textAlignXRightJustify,
			textAlignY,
			textWidth, textHeight });
		promptText->setFont(promptFont);
		promptText->setHJustify(Field::justify_t::RIGHT);
		promptText->setVJustify(Field::justify_t::CENTER);
		promptText->setColor(promptTextColor);

		textAlignXRightJustify = interactGlyph2->pos.x - 6 - textWidth;
		textAlignY = interactGlyph2->pos.y - 4;
		promptText = promptFrame->addField("txt 2", 32);
		promptText->setText(language[4040]);
		promptText->setSize(SDL_Rect{
			textAlignXRightJustify,
			textAlignY,
			textWidth, textHeight });
		promptText->setFont(promptFont);
		promptText->setHJustify(Field::justify_t::RIGHT);
		promptText->setVJustify(Field::justify_t::CENTER);
		promptText->setColor(promptTextColor);

		int textAlignXLeftJustify = interactGlyph3->pos.x + interactGlyph3->pos.w + 6;
		textAlignY = interactGlyph3->pos.y - 4;
		promptText = promptFrame->addField("txt 3", 32);
		promptText->setText("Equip");
		promptText->setSize(SDL_Rect{
			textAlignXLeftJustify,
			textAlignY,
			textWidth, textHeight });
		promptText->setFont(promptFont);
		promptText->setHJustify(Field::justify_t::LEFT);
		promptText->setVJustify(Field::justify_t::CENTER);
		promptText->setColor(promptTextColor);

		textAlignXLeftJustify = interactGlyph4->pos.x + interactGlyph4->pos.w + 6;
		textAlignY = interactGlyph4->pos.y - 4;
		promptText = promptFrame->addField("txt 4", 32);
		promptText->setText("Drop");
		promptText->setSize(SDL_Rect{
			textAlignXLeftJustify,
			textAlignY,
			textWidth, textHeight });
		promptText->setFont(promptFont);
		promptText->setHJustify(Field::justify_t::LEFT);
		promptText->setVJustify(Field::justify_t::CENTER);
		promptText->setColor(promptTextColor);

		bottomCenter->pos.y = interactGlyph4->pos.y + interactGlyph4->pos.h + 8;
		bottomLeft->pos.y = bottomCenter->pos.y;
		bottomRight->pos.y = bottomCenter->pos.y;

		promptSize.h = bottomCenter->pos.y + bottomCenter->pos.h;

		promptFrame->setSize(promptSize);
		middleCenter->pos.h = bottomCenter->pos.y - 4;
		middleLeft->pos.h = bottomCenter->pos.y;
		middleRight->pos.h = bottomCenter->pos.y;
		middleBottom->pos.y = bottomCenter->pos.y - 2;
	}
}

void drawCharacterPreview(const int player, SDL_Rect pos, int fov, view_t& view, real_t offsetyaw)
{
	auto ofov = ::fov;
	::fov = fov;

	//TempTexture* minimapTexture = new TempTexture();

	if ( players[player] != nullptr && players[player]->entity != nullptr )
	{
		if ( !softwaremode )
		{
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		//TODO: These two NOT PLAYERSWAP
		//camera.x=players[player]->x/16.0+.5*cos(players[player]->yaw)-.4*sin(players[player]->yaw);
		//camera.y=players[player]->y/16.0+.5*sin(players[player]->yaw)+.4*cos(players[player]->yaw);
		view.x = players[player]->entity->x / 16.0 + (.92 * cos(offsetyaw));
		view.y = players[player]->entity->y / 16.0 + (.92 * sin(offsetyaw));
		view.z = players[player]->entity->z * 2;
		//camera.ang=atan2(players[player]->y/16.0-camera.y,players[player]->x/16.0-camera.x); //TODO: _NOT_ PLAYERSWAP
		view.ang = (offsetyaw - PI); //5 * PI / 4;
		view.vang = PI / 20;

		view.winx = pos.x;
		// winy modification required due to new frame scaling method d49b1a5f34667432f2a2bd754c0abca3a09227c8
		view.winy = pos.y + (yres - Frame::virtualScreenY); 
		//view.winx = x1 + 8;
		//view.winy = y1 + 8;

		view.winw = pos.w;
		view.winh = pos.h;
		bool b = players[player]->entity->flags[BRIGHT];
		players[player]->entity->flags[BRIGHT] = true;
		if ( !players[player]->entity->flags[INVISIBLE] )
		{
			glDrawVoxel(&view, players[player]->entity, REALCOLORS);
		}
		players[player]->entity->flags[BRIGHT] = b;
		int c = 0;
		if ( multiplayer != CLIENT )
		{
			for ( node_t* node = players[player]->entity->children.first; node != nullptr; node = node->next )
			{
				if ( c == 0 )
				{
					c++;
					continue;
				}
				Entity* entity = (Entity*)node->element;
				if ( !entity->flags[INVISIBLE] )
				{
					b = entity->flags[BRIGHT];
					entity->flags[BRIGHT] = true;
					glDrawVoxel(&view, entity, REALCOLORS);
					entity->flags[BRIGHT] = b;
				}
				c++;
			}
			for ( node_t* node = map.entities->first; node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( (Sint32)entity->getUID() == -4 )
				{
					glDrawSprite(&view, entity, REALCOLORS);
				}
			}
		}
		else
		{
			for ( node_t* node = map.entities->first; node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( (entity->behavior == &actPlayerLimb && entity->skill[2] == player && !entity->flags[INVISIBLE]) || (Sint32)entity->getUID() == -4 )
				{
					b = entity->flags[BRIGHT];
					entity->flags[BRIGHT] = true;
					if ( (Sint32)entity->getUID() == -4 )
					{
						glDrawSprite(&view, entity, REALCOLORS);
					}
					else
					{
						glDrawVoxel(&view, entity, REALCOLORS);
					}
					entity->flags[BRIGHT] = b;
				}
			}
		}
	}
	::fov = ofov;
}

Player::SkillSheet_t::SkillSheetData_t Player::SkillSheet_t::skillSheetData;
void Player::SkillSheet_t::loadSkillSheetJSON()
{
	if ( !PHYSFS_getRealDir("/data/skillsheet_entries.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/skillsheet_entries.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/skillsheet_entries.json");
		inputPath.append("/data/skillsheet_entries.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);
			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( d.HasMember("skills") )
				{
					auto& allEntries = skillSheetData.skillEntries;
					allEntries.clear();
					for ( rapidjson::Value::ConstValueIterator itr = d["skills"].Begin();
						itr != d["skills"].End(); ++itr )
					{
						allEntries.push_back(SkillSheetData_t::SkillEntry_t());
						auto& entry = allEntries[allEntries.size() - 1];
						if ( (*itr).HasMember("name") )
						{
							entry.name = (*itr)["name"].GetString();
						}
						if ( (*itr).HasMember("id") )
						{
							entry.skillId = (*itr)["id"].GetInt();
						}
						if ( (*itr).HasMember("icon_base_path") )
						{
							entry.skillIconPath = (*itr)["icon_base_path"].GetString();
						}
						if ( (*itr).HasMember("icon_legend_path") )
						{
							entry.skillIconPathLegend = (*itr)["icon_legend_path"].GetString();
						}
						if ( (*itr).HasMember("icon_stat_path") )
						{
							entry.statIconPath = (*itr)["icon_stat_path"].GetString();
						}
						if ( (*itr).HasMember("description") )
						{
							entry.description = (*itr)["description"].GetString();
						}
						if ( (*itr).HasMember("legend_text") )
						{
							entry.legendaryDescription = (*itr)["legend_text"].GetString();
						}
						if ( (*itr).HasMember("effects") )
						{
							for ( rapidjson::Value::ConstValueIterator eff_itr = (*itr)["effects"].Begin(); eff_itr != (*itr)["effects"].End(); ++eff_itr )
							{
								entry.effects.push_back(SkillSheetData_t::SkillEntry_t::SkillEffect_t());
								auto& effect = entry.effects[entry.effects.size() - 1];
								effect.tag = (*eff_itr)["tag"].GetString();
								effect.title = (*eff_itr)["title"].GetString();
								effect.rawValue = (*eff_itr)["value"].GetString();
								effect.valueCustomWidthOffset = 0;
								if ( (*eff_itr).HasMember("custom_value_width_offset") )
								{
									effect.valueCustomWidthOffset = (*eff_itr)["custom_value_width_offset"].GetInt();
								}
								if ( (*eff_itr).HasMember("auto_resize_value") )
								{
									effect.bAllowAutoResizeValue = (*eff_itr)["auto_resize_value"].GetBool();
								}
								else
								{
									effect.bAllowAutoResizeValue = false;
								}
								if ( (*eff_itr).HasMember("realtime_update") )
								{
									effect.bAllowRealtimeUpdate = (*eff_itr)["realtime_update"].GetBool();
								}
								else
								{
									effect.bAllowRealtimeUpdate = true;
								}
							}
						}
						if ( (*itr).HasMember("effects_position") )
						{
							for ( rapidjson::Value::ConstMemberIterator effPos_itr = (*itr)["effects_position"].MemberBegin(); 
								effPos_itr != (*itr)["effects_position"].MemberEnd(); ++effPos_itr )
							{
								std::string memberName = effPos_itr->name.GetString();
								if ( memberName == "effect_start_offset_x" )
								{
									entry.effectStartOffsetX = effPos_itr->value.GetInt();
								}
								else if ( memberName == "effect_background_offset_x" )
								{
									entry.effectBackgroundOffsetX = effPos_itr->value.GetInt();
								}
								else if ( memberName == "effect_background_width" )
								{
									entry.effectBackgroundWidth = effPos_itr->value.GetInt();
								}
							}
						}
					}
				}
				if ( d.HasMember("window_scaling") )
				{
					if ( d["window_scaling"].HasMember("standard_scale_modifier_x") )
					{
						windowHeightScaleX = d["window_scaling"]["standard_scale_modifier_x"].GetDouble();
					}
					if ( d["window_scaling"].HasMember("standard_scale_modifier_y") )
					{
						windowHeightScaleY = d["window_scaling"]["standard_scale_modifier_y"].GetDouble();
					}
					if ( d["window_scaling"].HasMember("compact_scale_modifier_x") )
					{
						windowCompactHeightScaleX = d["window_scaling"]["compact_scale_modifier_x"].GetDouble();
					}
					if ( d["window_scaling"].HasMember("compact_scale_modifier_y") )
					{
						windowCompactHeightScaleY = d["window_scaling"]["compact_scale_modifier_y"].GetDouble();
					}
				}
				if ( d.HasMember("skill_select_images") )
				{
					if ( d["skill_select_images"].HasMember("highlight_left") )
					{
						skillSheetData.highlightSkillImg = d["skill_select_images"]["highlight_left"].GetString();
					}
					if ( d["skill_select_images"].HasMember("selected_left") )
					{
						skillSheetData.selectSkillImg = d["skill_select_images"]["selected_left"].GetString();
					}
					if ( d["skill_select_images"].HasMember("highlight_right") )
					{
						skillSheetData.highlightSkillImg_Right = d["skill_select_images"]["highlight_right"].GetString();
					}
					if ( d["skill_select_images"].HasMember("selected_right") )
					{
						skillSheetData.selectSkillImg_Right = d["skill_select_images"]["selected_right"].GetString();
					}
				}
				if ( d.HasMember("skill_background_icons") )
				{
					if ( d["skill_background_icons"].HasMember("default") )
					{
						skillSheetData.iconBgPathDefault = d["skill_background_icons"]["default"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("default_selected") )
					{
						skillSheetData.iconBgSelectedPathDefault = d["skill_background_icons"]["default_selected"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("novice") )
					{
						skillSheetData.iconBgPathNovice = d["skill_background_icons"]["novice"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("novice_selected") )
					{
						skillSheetData.iconBgSelectedPathNovice = d["skill_background_icons"]["novice_selected"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("expert") )
					{
						skillSheetData.iconBgPathExpert = d["skill_background_icons"]["expert"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("expert_selected") )
					{
						skillSheetData.iconBgSelectedPathExpert = d["skill_background_icons"]["expert_selected"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("legend") )
					{
						skillSheetData.iconBgPathLegend = d["skill_background_icons"]["legend"].GetString();
					}
					if ( d["skill_background_icons"].HasMember("legend_selected") )
					{
						skillSheetData.iconBgSelectedPathLegend = d["skill_background_icons"]["legend_selected"].GetString();
					}
				}
				if ( d.HasMember("alchemy_potion_names_to_filter") )
				{
					skillSheetData.potionNamesToFilter.clear();
					if ( d["alchemy_potion_names_to_filter"].IsString() )
					{
						skillSheetData.potionNamesToFilter.push_back(d["alchemy_potion_names_to_filter"].GetString());
					}
					else if ( d["alchemy_potion_names_to_filter"].IsArray() )
					{
						for ( rapidjson::Value::ConstValueIterator pot_itr = d["alchemy_potion_names_to_filter"].Begin(); 
							pot_itr != d["alchemy_potion_names_to_filter"].End(); ++pot_itr )
						{
							skillSheetData.potionNamesToFilter.push_back(pot_itr->GetString());
						}
					}
				}
				if ( d.HasMember("colors") )
				{
					if ( d["colors"].HasMember("default") )
					{
						skillSheetData.defaultTextColor = SDL_MapRGBA(mainsurface->format,
							d["colors"]["default"]["r"].GetInt(),
							d["colors"]["default"]["g"].GetInt(),
							d["colors"]["default"]["b"].GetInt(),
							d["colors"]["default"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("novice") )
					{
						skillSheetData.noviceTextColor = SDL_MapRGBA(mainsurface->format,
							d["colors"]["novice"]["r"].GetInt(),
							d["colors"]["novice"]["g"].GetInt(),
							d["colors"]["novice"]["b"].GetInt(),
							d["colors"]["novice"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("expert") )
					{
						skillSheetData.expertTextColor = SDL_MapRGBA(mainsurface->format,
							d["colors"]["expert"]["r"].GetInt(),
							d["colors"]["expert"]["g"].GetInt(),
							d["colors"]["expert"]["b"].GetInt(),
							d["colors"]["expert"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("legend") )
					{
						skillSheetData.legendTextColor = SDL_MapRGBA(mainsurface->format,
							d["colors"]["legend"]["r"].GetInt(),
							d["colors"]["legend"]["g"].GetInt(),
							d["colors"]["legend"]["b"].GetInt(),
							d["colors"]["legend"]["a"].GetInt());
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}

	if ( !PHYSFS_getRealDir("/data/skillsheet_leadership_entries.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/skillsheet_leadership_entries.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/skillsheet_leadership_entries.json");
		inputPath.append("/data/skillsheet_leadership_entries.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( d.HasMember("leadership_allies_base") )
				{
					auto& allyTable = skillSheetData.leadershipAllyTableBase;
					allyTable.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["leadership_allies_base"].MemberBegin();
						itr != d["leadership_allies_base"].MemberEnd(); ++itr )
					{
						std::string monsterName = itr->name.GetString();
						int monsterType = -1;
						for ( int i = 0; i < NUMMONSTERS; ++i )
						{
							if ( monsterName.compare(monstertypename[i]) == 0 )
							{
								monsterType = i;
								break;
							}
						}
						if ( monsterType < 0 ) { continue; }
						if ( itr->value.IsArray() )
						{
							for ( rapidjson::Value::ConstValueIterator ally_itr = itr->value.Begin();
								ally_itr != itr->value.End(); ++ally_itr )
							{
								std::string allyName = ally_itr->GetString();
								int allyType = -1;
								for ( int i = 0; i < NUMMONSTERS; ++i )
								{
									if ( allyName.compare(monstertypename[i]) == 0 )
									{
										allyType = i;
										break;
									}
								}
								if ( allyType >= 0 )
								{
									allyTable[(Monster)monsterType].push_back((Monster)allyType);
								}
							}
						}
					}
				}
				if ( d.HasMember("leadership_allies_legendary") )
				{
					auto& allyTable = skillSheetData.leadershipAllyTableLegendary;
					allyTable.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["leadership_allies_legendary"].MemberBegin();
						itr != d["leadership_allies_legendary"].MemberEnd(); ++itr )
					{
						std::string monsterName = itr->name.GetString();
						int monsterType = -1;
						for ( int i = 0; i < NUMMONSTERS; ++i )
						{
							if ( monsterName.compare(monstertypename[i]) == 0 )
							{
								monsterType = i;
								break;
							}
						}
						if ( monsterType < 0 ) { continue; }
						if ( itr->value.IsArray() )
						{
							for ( rapidjson::Value::ConstValueIterator ally_itr = itr->value.Begin();
								ally_itr != itr->value.End(); ++ally_itr )
							{
								std::string allyName = ally_itr->GetString();
								int allyType = -1;
								for ( int i = 0; i < NUMMONSTERS; ++i )
								{
									if ( allyName.compare(monstertypename[i]) == 0 )
									{
										allyType = i;
										break;
									}
								}
								if ( allyType >= 0 )
								{
									allyTable[(Monster)monsterType].push_back((Monster)allyType);
								}
							}
						}
					}
				}
				if ( d.HasMember("leadership_allies_unique_recruits") )
				{
					auto& allyTable = skillSheetData.leadershipAllyTableSpecialRecruitment;
					allyTable.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["leadership_allies_unique_recruits"].MemberBegin();
						itr != d["leadership_allies_unique_recruits"].MemberEnd(); ++itr )
					{
						std::string monsterName = itr->name.GetString();
						int monsterType = -1;
						for ( int i = 0; i < NUMMONSTERS; ++i )
						{
							if ( monsterName.compare(monstertypename[i]) == 0 )
							{
								monsterType = i;
								break;
							}
						}
						if ( monsterType < 0 ) { continue; }
						if ( itr->value.IsArray() )
						{
							for ( rapidjson::Value::ConstValueIterator ally_itr = itr->value.Begin();
								ally_itr != itr->value.End(); ++ally_itr )
							{
								for ( rapidjson::Value::ConstMemberIterator entry_itr = ally_itr->MemberBegin();
									entry_itr != ally_itr->MemberEnd(); ++entry_itr )
								{
									std::string allyName = entry_itr->name.GetString();
									int allyType = -1;
									for ( int i = 0; i < NUMMONSTERS; ++i )
									{
										if ( allyName.compare(monstertypename[i]) == 0 )
										{
											allyType = i;
											break;
										}
									}
									if ( allyType >= 0 )
									{
										allyTable[(Monster)monsterType].push_back(std::make_pair((Monster)allyType, entry_itr->value.GetString()));
									}
								}
							}
						}
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
}

void loadHUDSettingsJSON()
{
	if ( !PHYSFS_getRealDir("/data/HUD_settings.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/HUD_settings.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/HUD_settings.json");
		inputPath.append("/data/HUD_settings.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.HasMember("version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( d.HasMember("hotbar_slot_opacity") )
				{
					hotbarSlotOpacity = d["hotbar_slot_opacity"].GetInt();
				}
				if ( d.HasMember("hotbar_selected_slot_opacity") )
				{
					hotbarSelectedSlotOpacity = d["hotbar_selected_slot_opacity"].GetInt();
				}
				if ( d.HasMember("selected_cursor_opacity") )
				{
					selectedCursorOpacity = d["selected_cursor_opacity"].GetInt();
				}
				if ( d.HasMember("selected_old_cursor_opacity") )
				{
					oldSelectedCursorOpacity = d["selected_old_cursor_opacity"].GetInt();
				}
				if ( d.HasMember("enemy_hp_bars") )
				{
					if ( d["enemy_hp_bars"].HasMember("world_height_offsets") )
					{
						enemyBarSettings.heightOffsets.clear();
						for ( rapidjson::Value::ConstMemberIterator itr = d["enemy_hp_bars"]["world_height_offsets"].MemberBegin();
							itr != d["enemy_hp_bars"]["world_height_offsets"].MemberEnd(); ++itr )
						{
							enemyBarSettings.heightOffsets[itr->name.GetString()] = itr->value.GetFloat();
						}
					}
					if ( d["enemy_hp_bars"].HasMember("screen_depth_distance_offset") )
					{
						enemyBarSettings.screenDistanceOffsets.clear();
						for ( rapidjson::Value::ConstMemberIterator itr = d["enemy_hp_bars"]["screen_depth_distance_offset"].MemberBegin();
							itr != d["enemy_hp_bars"]["screen_depth_distance_offset"].MemberEnd(); ++itr )
						{
							enemyBarSettings.screenDistanceOffsets[itr->name.GetString()] = itr->value.GetFloat();
						}
					}
					if ( d["enemy_hp_bars"].HasMember("monster_bar_width_to_hp_intervals") )
					{
						EnemyHPDamageBarHandler::widthHealthBreakpointsMonsters.clear();
						for ( rapidjson::Value::ConstValueIterator itr = d["enemy_hp_bars"]["monster_bar_width_to_hp_intervals"].Begin();
							itr != d["enemy_hp_bars"]["monster_bar_width_to_hp_intervals"].End(); ++itr )
						{
							// you need to FindMember() if getting objects from an array...
							auto widthPercentMember = itr->FindMember("width_percent");
							auto hpThresholdMember = itr->FindMember("hp_threshold");
							if ( !widthPercentMember->value.IsInt() || !hpThresholdMember->value.IsInt() )
							{
								printlog("[JSON]: Error: Enemy bar HP or width was not int!");
								continue;
							}
							real_t widthPercent = widthPercentMember->value.GetInt();
							widthPercent /= 100.0;
							int hpThreshold = hpThresholdMember->value.GetInt();
							EnemyHPDamageBarHandler::widthHealthBreakpointsMonsters.push_back(
								std::make_pair(widthPercent, hpThreshold));
						}
					}
					if ( d["enemy_hp_bars"].HasMember("furniture_bar_width_to_hp_intervals") )
					{
						EnemyHPDamageBarHandler::widthHealthBreakpointsFurniture.clear();
						for ( rapidjson::Value::ConstValueIterator itr = d["enemy_hp_bars"]["furniture_bar_width_to_hp_intervals"].Begin();
							itr != d["enemy_hp_bars"]["furniture_bar_width_to_hp_intervals"].End(); ++itr )
						{
							// you need to FindMember() if getting objects from an array...
							auto widthPercentMember = itr->FindMember("width_percent");
							auto hpThresholdMember = itr->FindMember("hp_threshold");
							if ( !widthPercentMember->value.IsInt() || !hpThresholdMember->value.IsInt() )
							{
								printlog("[JSON]: Error: Enemy bar HP or width was not int!");
								continue;
							}
							real_t widthPercent = widthPercentMember->value.GetInt();
							widthPercent /= 100.0;
							int hpThreshold = hpThresholdMember->value.GetInt();
							EnemyHPDamageBarHandler::widthHealthBreakpointsFurniture.push_back(
								std::make_pair(widthPercent, hpThreshold));
						}
					}
					if ( d["enemy_hp_bars"].HasMember("monster_bar_lifetime_ticks") )
					{
						EnemyHPDamageBarHandler::maxTickLifetime = d["enemy_hp_bars"]["monster_bar_lifetime_ticks"].GetInt();
					}
					if ( d["enemy_hp_bars"].HasMember("furniture_bar_lifetime_ticks") )
					{
						EnemyHPDamageBarHandler::maxTickFurnitureLifetime = d["enemy_hp_bars"]["furniture_bar_lifetime_ticks"].GetInt();
					}
				}
				if ( d.HasMember("colors") )
				{
					if ( d["colors"].HasMember("itemmenu_heading_text") )
					{
						hudColors.itemContextMenuHeadingText = SDL_MapRGBA(mainsurface->format,
							d["colors"]["itemmenu_heading_text"]["r"].GetInt(),
							d["colors"]["itemmenu_heading_text"]["g"].GetInt(),
							d["colors"]["itemmenu_heading_text"]["b"].GetInt(),
							d["colors"]["itemmenu_heading_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_option_text") )
					{
						hudColors.itemContextMenuOptionText = SDL_MapRGBA(mainsurface->format,
							d["colors"]["itemmenu_option_text"]["r"].GetInt(),
							d["colors"]["itemmenu_option_text"]["g"].GetInt(),
							d["colors"]["itemmenu_option_text"]["b"].GetInt(),
							d["colors"]["itemmenu_option_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_selected_text") )
					{
						hudColors.itemContextMenuOptionSelectedText = SDL_MapRGBA(mainsurface->format,
							d["colors"]["itemmenu_selected_text"]["r"].GetInt(),
							d["colors"]["itemmenu_selected_text"]["g"].GetInt(),
							d["colors"]["itemmenu_selected_text"]["b"].GetInt(),
							d["colors"]["itemmenu_selected_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_selected_img") )
					{
						hudColors.itemContextMenuOptionSelectedImg = SDL_MapRGBA(mainsurface->format,
							d["colors"]["itemmenu_selected_img"]["r"].GetInt(),
							d["colors"]["itemmenu_selected_img"]["g"].GetInt(),
							d["colors"]["itemmenu_selected_img"]["b"].GetInt(),
							d["colors"]["itemmenu_selected_img"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_option_img") )
					{
						hudColors.itemContextMenuOptionImg = SDL_MapRGBA(mainsurface->format,
							d["colors"]["itemmenu_option_img"]["r"].GetInt(),
							d["colors"]["itemmenu_option_img"]["g"].GetInt(),
							d["colors"]["itemmenu_option_img"]["b"].GetInt(),
							d["colors"]["itemmenu_option_img"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_neutral_text") )
					{
						hudColors.characterSheetNeutral = SDL_MapRGBA(mainsurface->format,
							d["colors"]["charsheet_neutral_text"]["r"].GetInt(),
							d["colors"]["charsheet_neutral_text"]["g"].GetInt(),
							d["colors"]["charsheet_neutral_text"]["b"].GetInt(),
							d["colors"]["charsheet_neutral_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_positive_text") )
					{
						hudColors.characterSheetGreen = SDL_MapRGBA(mainsurface->format,
							d["colors"]["charsheet_positive_text"]["r"].GetInt(),
							d["colors"]["charsheet_positive_text"]["g"].GetInt(),
							d["colors"]["charsheet_positive_text"]["b"].GetInt(),
							d["colors"]["charsheet_positive_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_negative_text") )
					{
						hudColors.characterSheetRed = SDL_MapRGBA(mainsurface->format,
							d["colors"]["charsheet_negative_text"]["r"].GetInt(),
							d["colors"]["charsheet_negative_text"]["g"].GetInt(),
							d["colors"]["charsheet_negative_text"]["b"].GetInt(),
							d["colors"]["charsheet_negative_text"]["a"].GetInt());
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
}

void createPlayerInventory(const int player)
{
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	Frame* frame = gui->addFrame(name);
	players[player]->inventoryUI.frame = frame;
	frame->setSize(SDL_Rect{ players[player]->camera_virtualx1(),
		players[player]->camera_virtualy1(),
		players[player]->camera_virtualWidth(),
		players[player]->camera_virtualHeight() });
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);
	frame->setInheritParentFrameOpacity(false);

	createInventoryTooltipFrame(player);

	SDL_Rect basePos{ 0, 0, 105 * 2, 224 * 2 };
	{
		auto bgFrame = frame->addFrame("inventory base");
		bgFrame->setSize(basePos);
		const auto bgSize = bgFrame->getSize();
		bgFrame->addImage(SDL_Rect{ 0, 0, bgSize.w, bgSize.h },
			SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255),
			"images/ui/Inventory/HUD_Inventory_Base_02.png", "inventory base img");
	}

	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	players[player]->inventoryUI.slotFrames.clear();

	const int baseSlotOffsetX = 4;
	const int baseSlotOffsetY = 0;
	SDL_Rect invSlotsPos{ 0, 202, basePos.w, 242 };
	{
		const auto invSlotsFrame = frame->addFrame("inventory slots");
		invSlotsFrame->setSize(invSlotsPos);

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };

		for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize);
			for ( int y = 0; y < players[player]->inventoryUI.getSizeY(); ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + (y * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);

				auto slotFrame = invSlotsFrame->addFrame(slotname);
				players[player]->inventoryUI.slotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);
				//slotFrame->setDisabled(true);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}
	}

	{
		SDL_Rect dollSlotsPos{ 0, 0, basePos.w, invSlotsPos.y };
		const auto dollSlotsFrame = frame->addFrame("paperdoll slots");
		dollSlotsFrame->setSize(dollSlotsPos);

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };

		for ( int x = Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_LEFT; x <= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT; ++x )
		{
			currentSlotPos.x = baseSlotOffsetX;
			if ( x == Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT )
			{
				currentSlotPos.x = baseSlotOffsetX + (4 * inventorySlotSize); // 4 slots over
			}

			for ( int y = Player::Inventory_t::PaperDollRows::DOLL_ROW_1; y <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5; ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + ((y - Player::Inventory_t::PaperDollRows::DOLL_ROW_1) * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);

				auto slotFrame = dollSlotsFrame->addFrame(slotname);
				players[player]->inventoryUI.slotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);
				//slotFrame->setDisabled(true);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}

		{
			auto charFrame = frame->addFrame("inventory character preview");
			auto charSize = dollSlotsPos;
			charSize.x += inventorySlotSize + baseSlotOffsetX + 4;
			charSize.w -= 2 * (inventorySlotSize + baseSlotOffsetX + 4);

			charFrame->setSize(charSize);
			charFrame->setTickCallback([](Widget& widget) {
				int player = widget.getOwner();
				if ( Input::inputs[player].analog("InventoryCharacterRotateLeft") )
				{
					camera_charsheet_offsetyaw -= 0.05;
				}
				else if ( Input::inputs[player].analog("InventoryCharacterRotateRight") )
				{
					camera_charsheet_offsetyaw += 0.05;
				}
				if ( camera_charsheet_offsetyaw > 2 * PI )
				{
					camera_charsheet_offsetyaw -= 2 * PI;
				}
				if ( camera_charsheet_offsetyaw < 0.0 )
				{
					camera_charsheet_offsetyaw += 2 * PI;
				}
				});
			charFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos) {
				drawCharacterPreview(widget.getOwner(), pos, 50, camera_charsheet, camera_charsheet_offsetyaw);
			});
			/*charFrame->addImage(SDL_Rect{ 0, 0, charSize.w, charSize.h },
				SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255),
				"images/system/white.png", "inventory character preview bg");*/
		}

		/*auto selectedFrame = dollSlotsFrame->addFrame("paperdoll selected item");
		selectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		selectedFrame->setDisabled(true);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
		selectedFrame->addImage(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "paperdoll selected highlight");*/
	}

	{
		auto selectedFrame = frame->addFrame("inventory selected item");
		selectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		selectedFrame->setDisabled(true);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
		selectedFrame->addImage(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "inventory selected highlight");


		auto oldSelectedFrame = frame->addFrame("inventory old selected item");
		oldSelectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		oldSelectedFrame->setDisabled(true);

		const int itemSpriteSize = players[player]->inventoryUI.getItemSpriteSize();
		SDL_Rect itemSpriteBorder{ 2, 2, itemSpriteSize, itemSpriteSize };

		color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
		auto oldImg = oldSelectedFrame->addImage(itemSpriteBorder,
			SDL_MapRGBA(mainsurface->format, 255, 255, 255, 128), "", "inventory old selected item");
		oldImg->disabled = true;
		oldSelectedFrame->addImage(SDL_Rect{ 0, 0, oldSelectedFrame->getSize().w, oldSelectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "inventory old selected highlight");

		auto bgFrame = frame->findFrame("inventory base");
		auto flourishFrame = frame->addFrame("inventory base flourish");
		flourishFrame->setSize(SDL_Rect{ (bgFrame->getSize().w / 2) - (122 / 2), 202 - 22 + 6, 122, 22 });
		auto flourishImg = flourishFrame->addImage(SDL_Rect{ 0, 0, flourishFrame->getSize().w, flourishFrame->getSize().h },
			SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255),
			"images/ui/Inventory/HUD_Inventory_Flourish_00.png", "inventory flourish img");

		auto oldCursorFrame = frame->addFrame("inventory old item cursor");
		oldCursorFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize + 16, inventorySlotSize + 16 });
		oldCursorFrame->setDisabled(true);
		oldCursorFrame->setHollow(true);
		color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, oldSelectedCursorOpacity);
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_TL.png", "inventory old cursor topleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_TR.png", "inventory old cursor topright");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_BL.png", "inventory old cursor bottomleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/SelectorGrey_BR.png", "inventory old cursor bottomright");

		auto cursorFrame = frame->addFrame("inventory selected item cursor");
		players[player]->inventoryUI.selectedItemCursorFrame = cursorFrame;
		cursorFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize + 16, inventorySlotSize + 16 });
		cursorFrame->setDisabled(true);
		cursorFrame->setHollow(true);
		color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, selectedCursorOpacity);
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_TL.png", "inventory selected cursor topleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_TR.png", "inventory selected cursor topright");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_BL.png", "inventory selected cursor bottomleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_BR.png", "inventory selected cursor bottomright");
	}

	{
		auto draggingInventoryItem = frame->addFrame("dragging inventory item");
		draggingInventoryItem->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		draggingInventoryItem->setDisabled(true);
		createPlayerInventorySlotFrameElements(draggingInventoryItem);
	}

	{
		// unused for now, only animating cycling items
		auto draggingInventoryItemOld = frame->addFrame("dragging inventory item old");
		const int itemSpriteSize = players[player]->inventoryUI.getItemSpriteSize();
		SDL_Rect draggingInventoryItemPos{ 2, 2, inventorySlotSize - 2, inventorySlotSize - 2 };
		const int alignOffset = (draggingInventoryItemPos.w - itemSpriteSize) / 2; // align the item sprite within the box by this offset to center
		draggingInventoryItemPos.x += alignOffset;
		draggingInventoryItemPos.y += alignOffset;
		draggingInventoryItemPos.w = itemSpriteSize;
		draggingInventoryItemPos.h = itemSpriteSize;
		draggingInventoryItemOld->setSize(draggingInventoryItemPos);
		draggingInventoryItemOld->setDisabled(true);
		
		SDL_Rect imgPos{ 0, 0, draggingInventoryItemOld->getSize().w, draggingInventoryItemOld->getSize().h };
		auto itemSprite = draggingInventoryItemOld->addImage(imgPos, 0xFFFFFFFF, "", "item sprite img");
	}
}

void Player::Inventory_t::updateSelectedSlotAnimation(int destx, int desty, int width, int height, bool usingMouse)
{
	if ( frame )
	{
		if ( selectedItemCursorFrame )
		{
			if ( usingMouse )
			{
				selectedItemCursorFrame->setSize(
					SDL_Rect{
					destx - cursor.cursorToSlotOffset,
					desty - cursor.cursorToSlotOffset,
					width + 2 * (cursor.cursorToSlotOffset + 1),
					height + 2 * (cursor.cursorToSlotOffset + 1)
				}
				);
				cursor.animateSetpointX = destx;
				cursor.animateSetpointY = desty;
				cursor.animateStartX = destx;
				cursor.animateStartY = desty;
			}
			else if ( cursor.animateSetpointX != destx || cursor.animateSetpointY != desty )
			{
				SDL_Rect size = selectedItemCursorFrame->getSize();
				cursor.animateStartX = size.x;
				cursor.animateStartY = size.y;
				size.w = width + 2 * (cursor.cursorToSlotOffset + 1);
				size.h = height + 2 * (cursor.cursorToSlotOffset + 1);
				selectedItemCursorFrame->setSize(size);
				cursor.animateSetpointX = destx;
				cursor.animateSetpointY = desty;
				cursor.animateX = 0.0;
				cursor.animateY = 0.0;
				cursor.lastUpdateTick = ticks;
			}
		}
	}
}

void Player::Inventory_t::updateItemContextMenu()
{
	bool& toggleclick = inputs.getUIInteraction(player.playernum)->toggleclick;
	bool& itemMenuOpen = inputs.getUIInteraction(player.playernum)->itemMenuOpen;
	int& itemMenuSelected = inputs.getUIInteraction(player.playernum)->itemMenuSelected;
	Uint32& itemMenuItem = inputs.getUIInteraction(player.playernum)->itemMenuItem;
	int& itemMenuX = inputs.getUIInteraction(player.playernum)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player.playernum)->itemMenuY;
	const Sint32 mousex = (inputs.getMouse(player.playernum, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	const Sint32 mousey = (inputs.getMouse(player.playernum, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;

	Item* item = uidToItem(itemMenuItem);
	if ( !interactFrame )
	{
		itemMenuOpen = false;
		return;
	}

	if ( !item || !itemMenuOpen )
	{
		itemMenuOpen = false;
		interactFrame->setDisabled(true);
		return;
	}

	interactFrame->setDisabled(false);

	auto options = getContextMenuOptionsForItem(player.playernum, item);
	std::vector<std::pair<Frame::image_t*, Field*>> optionFrames;
	auto glyph1 = interactFrame->findImage("glyph 1");
	auto option1 = interactFrame->findField("interact option 1");
	auto glyph2 = interactFrame->findImage("glyph 2");
	auto option2 = interactFrame->findField("interact option 2");
	auto glyph3 = interactFrame->findImage("glyph 3");
	auto option3 = interactFrame->findField("interact option 3");
	auto glyph4 = interactFrame->findImage("glyph 4");
	auto option4 = interactFrame->findField("interact option 4");
	auto glyph5 = interactFrame->findImage("glyph 5");
	auto option5 = interactFrame->findField("interact option 5");
	if ( glyph1 && option1 )
	{
		optionFrames.push_back(std::make_pair(glyph1, option1));
	}
	if ( glyph2 && option2 )
	{
		optionFrames.push_back(std::make_pair(glyph2, option2));
	}
	if ( glyph3 && option3 )
	{
		optionFrames.push_back(std::make_pair(glyph3, option3));
	}
	if ( glyph4 && option4 )
	{
		optionFrames.push_back(std::make_pair(glyph4, option4));
	}
	if ( glyph5 && option5 )
	{
		optionFrames.push_back(std::make_pair(glyph5, option5));
	}

	auto highlightImage = interactFrame->findImage("interact selected highlight");
	highlightImage->disabled = true;
	highlightImage->color = hudColors.itemContextMenuOptionSelectedImg;
	
	size_t index = 0;
	unsigned int maxWidth = 0;
	if ( auto interactText = interactFrame->findField("interact text") )
	{
		interactText->setColor(hudColors.itemContextMenuHeadingText);
		if ( auto textGet = Text::get(interactText->getText(), interactText->getFont(),
			makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
		{
			maxWidth = textGet->getWidth();
		}
	}
	int maxHeight = 0;
	const int textPaddingX = 8;

	for ( auto& optionPair : optionFrames )
	{
		auto& img = optionPair.first;
		auto& txt = optionPair.second;
		txt->setColor(hudColors.itemContextMenuOptionText);
		if ( index >= options.size() )
		{
			txt->setDisabled(true);
			img->disabled = true;
			continue;
		}

		txt->setDisabled(false);
		img->disabled = true;
		auto promptType = options[index];
		
		// in-case we want controller support here.
		/*switch ( promptType ) 
		{
			case PROMPT_EQUIP:
			case PROMPT_UNEQUIP:
			case PROMPT_SPELL_EQUIP:
				img->path = Input::inputs[player.playernum].getGlyphPathForInput("MenuAlt1");
				break;
			case PROMPT_APPRAISE:
				img->path = Input::inputs[player.playernum].getGlyphPathForInput("MenuAlt2");
				break;
			case PROMPT_DROP:
				img->path = Input::inputs[player.playernum].getGlyphPathForInput("MenuCancel");
				break;
			default:
				img->path = Input::inputs[player.playernum].getGlyphPathForInput("MenuConfirm");
				break;
		}

		if ( img->path == "" )
		{
			img->disabled = true;
		}*/

		txt->setText(getContextMenuLangEntry(player.playernum, promptType, *item));
		if ( auto textGet = Text::get(txt->getText(), txt->getFont(),
			makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255)) )
		{
			maxWidth = std::max(textGet->getWidth(), maxWidth);
			
			SDL_Rect size = txt->getSize();
			size.w = textGet->getWidth();
			if ( img->disabled )
			{
				size.x = textPaddingX;
				txt->setHJustify(Field::justify_t::CENTER);
			}
			else
			{
				size.x = img->pos.x + img->pos.w + textPaddingX;
				txt->setHJustify(Field::justify_t::LEFT);
			}
			txt->setSize(size);
		}
		
		maxHeight = std::max(img->pos.y + img->pos.h, maxHeight);
		++index;
	}

	const int rightClickProtectBuffer = (right_click_protect ? 0 : 10);

	index = 0;
	for ( auto& optionPair : optionFrames )
	{
		auto& img = optionPair.first;
		auto& txt = optionPair.second;

		if ( txt->getHJustify() == Field::justify_t::CENTER )
		{
			SDL_Rect size = txt->getSize();
			size.w = maxWidth;
			txt->setSize(size);
		}

		if ( index < options.size() )
		{
			auto ml = interactFrame->findImage("interact middle left");
			SDL_Rect absoluteSize = txt->getAbsoluteSize();
			absoluteSize.x -= (4 + ml->pos.w) + rightClickProtectBuffer;
			absoluteSize.w += ((4 + ml->pos.w) * 2 + rightClickProtectBuffer);
			absoluteSize.y += 4;
			absoluteSize.h -= 4;
			if ( mousex >= absoluteSize.x && mousex < absoluteSize.x + absoluteSize.w
				&& mousey >= absoluteSize.y && mousey < absoluteSize.y + absoluteSize.h )
			{
				itemMenuSelected = index;
			}
		}
		++index;
	}

	const int textStartX = (glyph1->disabled ? 0 : glyph1->pos.x + glyph1->pos.w) + textPaddingX;
	const int frameWidth = maxWidth + textStartX + textPaddingX;

	const int posX = inputs.getMouse(player.playernum, Inputs::MouseInputs::X) * (real_t)Frame::virtualScreenX / xres;
	const int posY = inputs.getMouse(player.playernum, Inputs::MouseInputs::Y) * (real_t)Frame::virtualScreenY / yres;
	SDL_Rect frameSize = interactFrame->getSize();
	frameSize.x = itemMenuX;
	frameSize.y = itemMenuY;

	// position the frame elements
	int interimHeight = 0;
	{
		auto tl = interactFrame->findImage("interact top left");
		auto tmid = interactFrame->findImage("interact top background");
		tmid->pos.w = frameWidth - tl->pos.w * 2;
		auto tr = interactFrame->findImage("interact top right");
		tr->pos.x = tmid->pos.x + tmid->pos.w;

		interimHeight = tmid->pos.y + tmid->pos.h;
		frameSize.w = tr->pos.x + tr->pos.w;
	}
	{
		const int middleOffsetY = 4;
		auto ml = interactFrame->findImage("interact middle left");
		ml->pos.h = maxHeight - interimHeight - middleOffsetY;
		auto mmid = interactFrame->findImage("interact middle background");
		mmid->pos.h = ml->pos.h;
		mmid->pos.w = frameWidth - (ml->pos.w) * 2;
		mmid->color = hudColors.itemContextMenuOptionImg;
		auto mr = interactFrame->findImage("interact middle right");
		mr->pos.h = ml->pos.h;
		mr->pos.x = mmid->pos.x + mmid->pos.w;

		interimHeight = mmid->pos.y + mmid->pos.h;
	}
	{
		auto bl = interactFrame->findImage("interact bottom left");
		bl->pos.y = interimHeight;
		auto bmid = interactFrame->findImage("interact bottom background");
		bmid->pos.y = interimHeight;
		bmid->pos.w = frameWidth - bl->pos.w * 2;
		auto br = interactFrame->findImage("interact bottom right");
		br->pos.y = interimHeight;
		br->pos.x = bmid->pos.x + bmid->pos.w;

		frameSize.h = br->pos.y + br->pos.h;
	}
	
	if ( auto interactText = interactFrame->findField("interact text") )
	{
		SDL_Rect size = interactText->getSize();
		size.x = 0;
		size.w = frameSize.w;
		interactText->setSize(size);
	}

	interactFrame->setSize(frameSize);

	SDL_Rect absoluteSize = interactFrame->getAbsoluteSize();
	// right click protect uses exact border, else there is 10 px buffer
	absoluteSize.x -= rightClickProtectBuffer;
	absoluteSize.w += rightClickProtectBuffer;
	if ( !(mousex >= absoluteSize.x && mousex < absoluteSize.x + absoluteSize.w
		&& mousey >= absoluteSize.y && mousey < absoluteSize.y + absoluteSize.h) )
	{
		itemMenuSelected = -1;
	}

	if ( itemMenuSelected >= 0 && itemMenuSelected < options.size() )
	{
		auto& txt = optionFrames[itemMenuSelected].second;
		txt->setColor(hudColors.itemContextMenuOptionSelectedText);
		SDL_Rect size = txt->getSize();
		size.x -= 4;
		size.w += 2 * 4;
		size.h += 2;
		highlightImage->pos = size;
		highlightImage->disabled = false;
	}

	bool activateSelection = false;
	if ( !inputs.bMouseRight(player.playernum) && !toggleclick )
	{
		activateSelection = true;
	}
	if ( activateSelection )
	{
		if ( itemMenuSelected >= 0 && itemMenuSelected < options.size() )
		{
			activateItemContextMenuOption(item, options[itemMenuSelected]);
		}
		//Close the menu.
		itemMenuOpen = false;
		itemMenuItem = 0;
	}
}

void Player::Inventory_t::activateItemContextMenuOption(Item* item, ItemContextMenuPrompts prompt)
{
	const int player = this->player.playernum;
	bool disableItemUsage = false;
	if ( players[player] && players[player]->entity )
	{
		if ( players[player]->entity->effectShapeshift != NOTHING )
		{
			// shape shifted, disable some items
			if ( !item->usableWhileShapeshifted(stats[player]) )
			{
				disableItemUsage = true;
			}
			if ( item->type == FOOD_CREAMPIE && (prompt == PROMPT_UNEQUIP || prompt == PROMPT_EQUIP) )
			{
				disableItemUsage = true;
			}
		}
		else
		{
			if ( itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
			{
				if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 )
				{
					disableItemUsage = true;
				}
			}
		}
	}

	if ( client_classes[player] == CLASS_SHAMAN )
	{
		if ( item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
		{
			disableItemUsage = true;
		}
	}

	//PROMPT_EQUIP,
	//PROMPT_UNEQUIP,
	//PROMPT_SPELL_EQUIP,
	//PROMPT_INTERACT,
	//PROMPT_EAT,
	//PROMPT_CONSUME,
//PROMPT_SPELL_QUICKCAST,
//PROMPT_INSPECT,
//PROMPT_SELL,
//PROMPT_BUY,
//PROMPT_STORE_CHEST,
//PROMPT_RETRIEVE_CHEST,
	//PROMPT_APPRAISE,
	//PROMPT_DROP
	if ( prompt == PROMPT_APPRAISE )
	{
		players[player]->inventoryUI.appraisal.appraiseItem(item);
		return;
	}
	else if ( prompt == PROMPT_DROP )
	{
		dropItem(item, player);
		return;
	}
	else if ( prompt == PROMPT_SELL )
	{
	}
	else if ( prompt == PROMPT_STORE_CHEST )
	{
	}
	else if ( prompt == PROMPT_GRAB )
	{
		inputs.getUIInteraction(player)->selectedItem = item;
		playSound(139, 64); // click sound
		inputs.getUIInteraction(player)->toggleclick = true;
		inputs.mouseClearLeft(player);
		return;
	}
	else if ( prompt == PROMPT_EAT )
	{
		if ( !disableItemUsage )
		{
			useItem(item, player);
		}
		return;
	}
	else if ( prompt == PROMPT_CONSUME )
	{
		// consume item
		if ( multiplayer == CLIENT )
		{
			strcpy((char*)net_packet->data, "FODA");
			SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
			SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
			SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
			SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
			SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
			net_packet->data[24] = item->identified;
			net_packet->data[25] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		item_FoodAutomaton(item, player);
		return;
	}
	else if ( prompt == PROMPT_INTERACT || prompt == PROMPT_INSPECT || prompt == PROMPT_TINKER )
	{
		if ( item->type == TOOL_ALEMBIC )
		{
			// not experimenting
			if ( !disableItemUsage )
			{
				GenericGUI[player].openGUI(GUI_TYPE_ALCHEMY, false, item);
			}
			else
			{
				messagePlayer(player, language[3432]); // unable to use in current form message.
			}
		}
		else if ( item->type == TOOL_TINKERING_KIT )
		{
			if ( !disableItemUsage )
			{
				GenericGUI[player].openGUI(GUI_TYPE_TINKERING, item);
			}
			else
			{
				messagePlayer(player, language[3432]); // unable to use in current form message.
			}
		}
		else
		{
			useItem(item, player);
		}
		return;
	}
	else if ( prompt == PROMPT_EQUIP 
		|| prompt == PROMPT_UNEQUIP
		|| prompt == PROMPT_SPELL_EQUIP )
	{
		if ( isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook )
		{
			return; // don't try to equip shields while casting
		}

		if ( !disableItemUsage )
		{
			if ( prompt == PROMPT_EQUIP
				|| prompt == PROMPT_UNEQUIP )
			{
				if ( items[item->type].item_slot == ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON
					|| itemCategory(item) == SPELLBOOK )
				{
					playerTryEquipItemAndUpdateServer(player, item, true);
					return;
				}
			}

			useItem(item, player);
			return;
		}
		else
		{
			if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
			{
				messagePlayer(player, language[3488]); // unable to use with current level.
			}
			else
			{
				messagePlayer(player, language[3432]); // unable to use in current form message.
			}
		}
		return;
	}
	else if ( prompt == PROMPT_SPELL_QUICKCAST )
	{
		if ( !disableItemUsage )
		{
			players[player]->magic.setQuickCastSpellFromInventory(item);
		}
		else
		{
			if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
			{
				messagePlayer(player, language[3488]); // unable to use with current level.
			}
			else
			{
				messagePlayer(player, language[3432]); // unable to use in current form message.
			}
		}
		return;
	}
}

void Player::Hotbar_t::updateSelectedSlotAnimation(int destx, int desty, int width, int height, bool usingMouse)
{
	if ( hotbarFrame )
	{
		if ( auto selectedSlotCursor = hotbarFrame->findFrame("shootmode selected item cursor") )
		{
			if ( usingMouse )
			{
				selectedSlotCursor->setSize(
					SDL_Rect{
					destx - shootmodeCursor.cursorToSlotOffset,
					desty - shootmodeCursor.cursorToSlotOffset,
					width + 2 * (shootmodeCursor.cursorToSlotOffset + 1),
					height + 2 * (shootmodeCursor.cursorToSlotOffset + 1)
				}
				);
				shootmodeCursor.animateSetpointX = destx;
				shootmodeCursor.animateSetpointY = desty;
				shootmodeCursor.animateStartX = destx;
				shootmodeCursor.animateStartY = desty;
			}
			else if ( shootmodeCursor.animateSetpointX != destx || shootmodeCursor.animateSetpointY != desty )
			{
				SDL_Rect size = selectedSlotCursor->getSize();
				shootmodeCursor.animateStartX = size.x;
				shootmodeCursor.animateStartY = size.y;
				size.w = width + 2 * (shootmodeCursor.cursorToSlotOffset + 1);
				size.h = height + 2 * (shootmodeCursor.cursorToSlotOffset + 1);
				selectedSlotCursor->setSize(size);
				shootmodeCursor.animateSetpointX = destx;
				shootmodeCursor.animateSetpointY = desty;
				shootmodeCursor.animateX = 0.0;
				shootmodeCursor.animateY = 0.0;
				shootmodeCursor.lastUpdateTick = ticks;
			}
		}
	}
}

void Player::Inventory_t::updateSelectedItemAnimation()
{
	if ( frame )
	{
		if ( auto selectedSlotFrame = frame->findFrame("inventory selected item") )
		{
			selectedSlotFrame->setDisabled(true);
		}
		if ( selectedItemCursorFrame )
		{
			selectedItemCursorFrame->setDisabled(true);
		}
	}
	if ( inputs.getUIInteraction(player.playernum)->selectedItem )
	{
		const real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		real_t setpointDiffX = fpsScale * std::max(.05, (1.0 - selectedItemAnimate.animateX)) / (5);
		real_t setpointDiffY = fpsScale * std::max(.05, (1.0 - selectedItemAnimate.animateY)) / (5);
		selectedItemAnimate.animateX += setpointDiffX;
		selectedItemAnimate.animateY += setpointDiffY;
		selectedItemAnimate.animateX = std::min(1.0, selectedItemAnimate.animateX);
		selectedItemAnimate.animateY = std::min(1.0, selectedItemAnimate.animateY);
	}
	else
	{
		selectedItemAnimate.animateX = 0.0;
		selectedItemAnimate.animateY = 0.0;
	}
}

void Player::Inventory_t::updateInventoryItemTooltip()
{
	if ( !tooltipFrame || !frame )
	{
		return;
	}

	auto& tooltipDisplay = this->itemTooltipDisplay;

	if ( static_cast<int>(tooltipFrame->getOpacity()) != tooltipDisplay.opacitySetpoint )
	{
		const real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		if ( tooltipDisplay.opacitySetpoint == 0 )
		{
			real_t setpointDiff = fpsScale * std::max(.05, (tooltipDisplay.opacityAnimate)) / (5);
			tooltipDisplay.opacityAnimate -= setpointDiff;
			tooltipDisplay.opacityAnimate = std::max(0.0, tooltipDisplay.opacityAnimate);
		}
		else
		{
			real_t setpointDiff = fpsScale * std::max(.05, (1.0 - tooltipDisplay.opacityAnimate)) / (1);
			tooltipDisplay.opacityAnimate += setpointDiff;
			tooltipDisplay.opacityAnimate = std::min(1.0, tooltipDisplay.opacityAnimate);
		}
		tooltipFrame->setOpacity(tooltipDisplay.opacityAnimate * 100);
	}

	if ( tooltipPromptFrame )
	{
		tooltipPromptFrame->setOpacity(tooltipFrame->getOpacity());
	}

	tooltipDisplay.expandSetpoint = tooltipDisplay.expanded ? 100 : 0;
	if ( static_cast<int>(tooltipDisplay.expandCurrent * 100) != tooltipDisplay.expandSetpoint )
	{
		const real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		if ( tooltipDisplay.expandSetpoint == 0 )
		{
			//real_t setpointDiff = fpsScale * std::max(.05, (tooltipDisplay.expandAnimate) / 50);
			//tooltipDisplay.expandAnimate -= setpointDiff;
			tooltipDisplay.expandAnimate -= 2 * fpsScale / 100.0;
			tooltipDisplay.expandAnimate = std::max(0.0, tooltipDisplay.expandAnimate);
		}
		else
		{
			//real_t setpointDiff = fpsScale * std::max(.05, (1.0 - tooltipDisplay.expandAnimate) / 50);
			//tooltipDisplay.expandAnimate += setpointDiff;
			tooltipDisplay.expandAnimate += 2 * fpsScale / 100.0;
			tooltipDisplay.expandAnimate = std::min(1.0, tooltipDisplay.expandAnimate);
		}
		double t = tooltipDisplay.expandAnimate;
		tooltipDisplay.expandCurrent = t * t * (3.0f - 2.0f * t); // bezier from 0 to width as t (0-1);
	}
}

void Player::Inventory_t::ItemTooltipDisplay_t::updateItem(const int player, Item* newItem)
{
	if ( newItem && player >= 0 && player < MAXPLAYERS && stats[player] )
	{
		uid = newItem->uid;
		type = newItem->type;
		status = newItem->status;
		beatitude = newItem->beatitude;
		count = newItem->count;
		appearance = newItem->appearance;
		identified = newItem->identified;

		if ( stats[player] )
		{
			playernum = player;
			playerLVL = stats[player]->LVL;
			playerEXP = stats[player]->EXP;
			playerSTR = statGetSTR(stats[player], players[player]->entity);
			playerDEX = statGetDEX(stats[player], players[player]->entity);
			playerCON = statGetCON(stats[player], players[player]->entity);
			playerINT = statGetINT(stats[player], players[player]->entity);
			playerPER = statGetPER(stats[player], players[player]->entity);
			playerCHR = statGetCHR(stats[player], players[player]->entity);
		}
	}
}

bool Player::Inventory_t::ItemTooltipDisplay_t::isItemSameAsCurrent(const int player, Item* newItem)
{
	if ( newItem && player >= 0 && player < MAXPLAYERS && stats[player] )
	{
		if ( newItem->uid == uid
			&& newItem->type == type
			&& newItem->status == status
			&& newItem->beatitude == beatitude
			&& newItem->count == count
			&& newItem->appearance == appearance
			&& newItem->identified == identified
			&& playernum == player
			&& playerLVL == stats[player]->LVL
			&& playerEXP == stats[player]->EXP
			&& playerSTR == statGetSTR(stats[player], players[player]->entity)
			&& playerDEX == statGetDEX(stats[player], players[player]->entity)
			&& playerCON == statGetCON(stats[player], players[player]->entity)
			&& playerINT == statGetINT(stats[player], players[player]->entity)
			&& playerPER == statGetPER(stats[player], players[player]->entity)
			&& playerCHR == statGetCHR(stats[player], players[player]->entity)
		)
		{
			return true;
		}
	}
	return false;
}

Player::Inventory_t::ItemTooltipDisplay_t::ItemTooltipDisplay_t()
{
	uid = 0;
	type = WOODEN_SHIELD;
	status = BROKEN;
	beatitude = 0;
	count = 0;
	appearance = 0;
	identified = false;

	playernum = -1;
	playerLVL = 0;
	playerEXP = 0;
	playerSTR = 0;
	playerDEX = 0;
	playerCON = 0;
	playerINT = 0;
	playerPER = 0;
	playerCHR = 0;
}

void Player::Inventory_t::updateCursor()
{
	if ( !frame )
	{
		return;
	}

	if ( auto oldSelectedSlotCursor = frame->findFrame("inventory old item cursor") )
	{
		if ( auto oldSelectedFrame = frame->findFrame("inventory old selected item") )
		{
			oldSelectedSlotCursor->setDisabled(oldSelectedFrame->isDisabled());

			if ( player.hotbar.hotbarFrame )
			{
				if ( auto highlight = oldSelectedFrame->findImage("inventory old selected highlight") )
				{
					highlight->disabled = false;
					if ( auto oldHotbarSelectedFrame = player.hotbar.hotbarFrame->findFrame("hotbar old selected item") )
					{
						if ( !oldHotbarSelectedFrame->isDisabled() )
						{
							oldSelectedSlotCursor->setDisabled(true);
							highlight->disabled = true;
						}
					}
				}
			}

			if ( !oldSelectedSlotCursor->isDisabled() )
			{
				SDL_Rect cursorSize = oldSelectedSlotCursor->getSize();
				cursorSize.x = (oldSelectedFrame->getSize().x - 1) - cursor.cursorToSlotOffset;
				cursorSize.y = (oldSelectedFrame->getSize().y - 1) - cursor.cursorToSlotOffset;
				oldSelectedSlotCursor->setSize(cursorSize);

				int offset = 8;// ((ticks - cursor.lastUpdateTick) % 50 < 25) ? largeOffset : smallOffset;

				Uint8 r, g, b, a;
				if ( auto tl = oldSelectedSlotCursor->findImage("inventory old cursor topleft") )
				{
					tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
					SDL_GetRGBA(tl->color, mainsurface->format, &r, &g, &b, &a);
					a = oldSelectedCursorOpacity;
					tl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
				if ( auto tr = oldSelectedSlotCursor->findImage("inventory old cursor topright") )
				{
					tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
					tr->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
				if ( auto bl = oldSelectedSlotCursor->findImage("inventory old cursor bottomleft") )
				{
					bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
					bl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
				if ( auto br = oldSelectedSlotCursor->findImage("inventory old cursor bottomright") )
				{
					br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
					br->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
			}
		}
	}

	if ( selectedItemCursorFrame )
	{
		SDL_Rect cursorSize = selectedItemCursorFrame->getSize();

		const int smallOffset = 2;
		const int largeOffset = 4;

		int offset = ((ticks - cursor.lastUpdateTick) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2) ? largeOffset : smallOffset;
		if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			if ( inputs.getUIInteraction(player.playernum)->selectedItem 
				|| inputs.getUIInteraction(player.playernum)->itemMenuOpen )
			{
				// animate cursor
			}
			else
			{
				offset = smallOffset; // don't animate while mouse normal hovering
			}
		}

		Uint8 r, g, b, a;
		if ( auto tl = selectedItemCursorFrame->findImage("inventory selected cursor topleft") )
		{
			tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
			SDL_GetRGBA(tl->color, mainsurface->format, &r, &g, &b, &a);
			a = selectedCursorOpacity;
			tl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto tr = selectedItemCursorFrame->findImage("inventory selected cursor topright") )
		{
			tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
			tr->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto bl = selectedItemCursorFrame->findImage("inventory selected cursor bottomleft") )
		{
			bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
			bl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto br = selectedItemCursorFrame->findImage("inventory selected cursor bottomright") )
		{
			br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
			br->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}

		SDL_Rect currentPos = selectedItemCursorFrame->getSize();
		const int offsetPosition = cursor.cursorToSlotOffset;
		if ( cursor.animateSetpointX - offsetPosition != currentPos.x
			|| cursor.animateSetpointY - offsetPosition != currentPos.y )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - cursor.animateX)) / (2.5);
			real_t setpointDiffY = fpsScale * std::max(.1, (1.0 - cursor.animateY)) / (2.5);
			cursor.animateX += setpointDiffX;
			cursor.animateY += setpointDiffY;
			cursor.animateX = std::min(1.0, cursor.animateX);
			cursor.animateY = std::min(1.0, cursor.animateY);

			int destX = cursor.animateSetpointX - cursor.animateStartX - offsetPosition;
			int destY = cursor.animateSetpointY - cursor.animateStartY - offsetPosition;

			currentPos.x = cursor.animateStartX + destX * cursor.animateX;
			currentPos.y = cursor.animateStartY + destY * cursor.animateY;
			selectedItemCursorFrame->setSize(currentPos);
			//messagePlayer(0, "%.2f | %.2f", inventory_t.selectedSlotAnimateX, setpointDiffX);
		}
	}
}

void Player::HUD_t::updateCursorAnimation(int destx, int desty, int width, int height, bool usingMouse)
{
	if ( cursorFrame )
	{
		if ( auto hudCursor = cursorFrame->findFrame("hud cursor") )
		{
			if ( usingMouse )
			{
				hudCursor->setSize(
					SDL_Rect{
					destx - cursor.cursorToSlotOffset,
					desty - cursor.cursorToSlotOffset,
					width + 2 * (cursor.cursorToSlotOffset + 1),
					height + 2 * (cursor.cursorToSlotOffset + 1)
				}
				);
				cursor.animateSetpointX = destx;
				cursor.animateSetpointY = desty;
				cursor.animateSetpointW = width;
				cursor.animateSetpointH = height;

				cursor.animateStartX = destx;
				cursor.animateStartY = desty;
				cursor.animateStartW = width;
				cursor.animateStartH = height;
			}
			else if ( cursor.animateSetpointX != destx || cursor.animateSetpointY != desty )
			{
				SDL_Rect size = hudCursor->getSize();
				cursor.animateStartX = size.x;
				cursor.animateStartY = size.y;
				cursor.animateStartW = size.w;
				cursor.animateStartH = size.h;

				hudCursor->setSize(size);
				cursor.animateSetpointX = destx;
				cursor.animateSetpointY = desty;
				cursor.animateSetpointW = width + 2 * (cursor.cursorToSlotOffset + 1);
				cursor.animateSetpointH = height + 2 * (cursor.cursorToSlotOffset + 1);

				cursor.animateX = 0.0;
				cursor.animateY = 0.0;
				cursor.animateW = 0.0;
				cursor.animateH = 0.0;

				cursor.lastUpdateTick = ticks;
			}
		}
	}
}

void Player::HUD_t::updateCursor()
{
	if ( !cursorFrame )
	{
		char name[32];
		snprintf(name, sizeof(name), "player hud cursor %d", player.playernum);
		cursorFrame = gui->addFrame(name);
		cursorFrame->setHollow(true);
		cursorFrame->setBorder(0);
		cursorFrame->setOwner(player.playernum);

		auto cursor = cursorFrame->addFrame("hud cursor");
		cursor->setHollow(true);
		cursor->setSize(SDL_Rect{ 0, 0, 0, 0 });
		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, selectedCursorOpacity);
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_TL.png", "hud cursor topleft");
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_TR.png", "hud cursor topright");
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_BL.png", "hud cursor bottomleft");
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "images/ui/Inventory/Selector_BR.png", "hud cursor bottomright");
	}

	cursorFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	if ( !players[player.playernum]->isLocalPlayer() 
		|| players[player.playernum]->shootmode 
		|| players[player.playernum]->GUI.bActiveModuleUsesInventory()
		|| players[player.playernum]->GUI.bActiveModuleHasNoCursor() )
	{
		// hide
		cursorFrame->setDisabled(true);
	}
	else
	{
		cursorFrame->setDisabled(false);
	}

	if ( auto hudCursor = cursorFrame->findFrame("hud cursor") )
	{
		SDL_Rect cursorSize = hudCursor->getSize();
		const int smallOffset = 2;
		const int largeOffset = 4;

		int offset = ((ticks - cursor.lastUpdateTick) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2) ? largeOffset : smallOffset;
		if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			//if ( inputs.getUIInteraction(player.playernum)->selectedItem
			//	|| inputs.getUIInteraction(player.playernum)->itemMenuOpen )
			//{
			//	// animate cursor
			//}
			//else
			{
				offset = smallOffset; // don't animate while mouse normal hovering
			}
		}

		Uint8 r, g, b, a;
		if ( auto tl = hudCursor->findImage("hud cursor topleft") )
		{
			tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
			SDL_GetRGBA(tl->color, mainsurface->format, &r, &g, &b, &a);
			a = selectedCursorOpacity;
			tl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto tr = hudCursor->findImage("hud cursor topright") )
		{
			tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
			tr->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto bl = hudCursor->findImage("hud cursor bottomleft") )
		{
			bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
			bl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto br = hudCursor->findImage("hud cursor bottomright") )
		{
			br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
			br->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}

		SDL_Rect currentPos = hudCursor->getSize();
		const int offsetPosition = cursor.cursorToSlotOffset;
		if ( cursor.animateSetpointX - offsetPosition != currentPos.x
			|| cursor.animateSetpointY - offsetPosition != currentPos.y )
		{
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - cursor.animateX)) / (2.5);
			real_t setpointDiffY = fpsScale * std::max(.1, (1.0 - cursor.animateY)) / (2.5);
			real_t setpointDiffW = fpsScale * std::max(.1, (1.0 - cursor.animateW)) / (2.5);
			real_t setpointDiffH = fpsScale * std::max(.1, (1.0 - cursor.animateH)) / (2.5);
			cursor.animateX += setpointDiffX;
			cursor.animateY += setpointDiffY;
			cursor.animateX = std::min(1.0, cursor.animateX);
			cursor.animateY = std::min(1.0, cursor.animateY);
			cursor.animateW += setpointDiffW;
			cursor.animateH += setpointDiffH;
			cursor.animateW = std::min(1.0, cursor.animateW);
			cursor.animateH = std::min(1.0, cursor.animateH);

			int destX = cursor.animateSetpointX - cursor.animateStartX - offsetPosition;
			int destY = cursor.animateSetpointY - cursor.animateStartY - offsetPosition;
			int destW = cursor.animateSetpointW - cursor.animateStartW;
			int destH = cursor.animateSetpointH - cursor.animateStartH;

			currentPos.x = cursor.animateStartX + destX * cursor.animateX;
			currentPos.y = cursor.animateStartY + destY * cursor.animateY;
			currentPos.w = cursor.animateStartW + destW * cursor.animateW;
			currentPos.h = cursor.animateStartH + destH * cursor.animateH;
			hudCursor->setSize(currentPos);
		}
	}
}

void Player::Hotbar_t::updateCursor()
{
	if ( !hotbarFrame )
	{
		return;
	}

	if ( auto oldSelectedSlotCursor = hotbarFrame->findFrame("hotbar old item cursor") )
	{
		if ( auto oldSelectedFrame = hotbarFrame->findFrame("hotbar old selected item") )
		{
			oldSelectedSlotCursor->setDisabled(oldSelectedFrame->isDisabled());

			if ( !oldSelectedSlotCursor->isDisabled() )
			{
				SDL_Rect cursorSize = oldSelectedSlotCursor->getSize();
				cursorSize.x = (oldSelectedFrame->getSize().x - 1) - shootmodeCursor.cursorToSlotOffset;
				cursorSize.y = (oldSelectedFrame->getSize().y - 1) - shootmodeCursor.cursorToSlotOffset;
				oldSelectedSlotCursor->setSize(cursorSize);

				int offset = 8;// ((ticks - shootmodeCursor.lastUpdateTick) % TICKS_PER_SECOND < 25) ? largeOffset : smallOffset;

				Uint8 r, g, b, a;
				if ( auto tl = oldSelectedSlotCursor->findImage("hotbar old cursor topleft") )
				{
					tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
					SDL_GetRGBA(tl->color, mainsurface->format, &r, &g, &b, &a);
					a = oldSelectedCursorOpacity;
					tl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
				if ( auto tr = oldSelectedSlotCursor->findImage("hotbar old cursor topright") )
				{
					tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
					tr->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
				if ( auto bl = oldSelectedSlotCursor->findImage("hotbar old cursor bottomleft") )
				{
					bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
					bl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
				if ( auto br = oldSelectedSlotCursor->findImage("hotbar old cursor bottomright") )
				{
					br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
					br->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
				}
			}
		}
	}

	if ( auto selectedSlotCursor = hotbarFrame->findFrame("shootmode selected item cursor") )
	{
		SDL_Rect cursorSize = selectedSlotCursor->getSize();

		const int smallOffset = 2;
		const int largeOffset = 4;

		int offset = ((ticks - shootmodeCursor.lastUpdateTick) % 50 < 25) ? largeOffset : smallOffset;
		if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			if ( inputs.getUIInteraction(player.playernum)->selectedItem )
			{
				//offset = largeOffset;
			}
			else
			{
				offset = smallOffset;
			}
		}

		Uint8 r, g, b, a;
		if ( auto tl = selectedSlotCursor->findImage("shootmode selected cursor topleft") )
		{
			tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
			SDL_GetRGBA(tl->color, mainsurface->format, &r, &g, &b, &a);
			a = selectedCursorOpacity;
			tl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto tr = selectedSlotCursor->findImage("shootmode selected cursor topright") )
		{
			tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
			tr->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto bl = selectedSlotCursor->findImage("shootmode selected cursor bottomleft") )
		{
			bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
			bl->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( auto br = selectedSlotCursor->findImage("shootmode selected cursor bottomright") )
		{
			br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
			br->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}

		SDL_Rect currentPos = selectedSlotCursor->getSize();
		const int offsetPosition = shootmodeCursor.cursorToSlotOffset;
		if ( shootmodeCursor.animateSetpointX - offsetPosition != currentPos.x
			|| shootmodeCursor.animateSetpointY - offsetPosition != currentPos.y )
		{
			auto& cursor = shootmodeCursor;
			const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - cursor.animateX)) / (2.5);
			real_t setpointDiffY = fpsScale * std::max(.1, (1.0 - cursor.animateY)) / (2.5);
			cursor.animateX += setpointDiffX;
			cursor.animateY += setpointDiffY;
			cursor.animateX = std::min(1.0, cursor.animateX);
			cursor.animateY = std::min(1.0, cursor.animateY);

			int destX = cursor.animateSetpointX - cursor.animateStartX - offsetPosition;
			int destY = cursor.animateSetpointY - cursor.animateStartY - offsetPosition;

			currentPos.x = cursor.animateStartX + destX * cursor.animateX;
			currentPos.y = cursor.animateStartY + destY * cursor.animateY;
			selectedSlotCursor->setSize(currentPos);
			//messagePlayer(0, "%.2f | %.2f", inventory_t.selectedSlotAnimateX, setpointDiffX);
		}
	}
}

void Player::Inventory_t::processInventory()
{
	if ( !frame )
	{
		createPlayerInventory(player.playernum);
	}

	frame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight()});

	bool tooltipWasDisabled = tooltipFrame->isDisabled();

	updateInventory();

	if ( tooltipWasDisabled && !tooltipFrame->isDisabled() )
	{
		tooltipFrame->setOpacity(0.0);
	}
}

void Player::HUD_t::resetBars()
{
	if ( xpFrame )
	{
		xpBar.animateSetpoint = std::min(100, stats[player.playernum]->EXP) * 10;
		xpBar.animateValue = xpBar.animateSetpoint;
		xpBar.animatePreviousSetpoint = xpBar.animateSetpoint;
		xpBar.animateState = ANIMATE_NONE;
		xpBar.xpLevelups = 0;
	}
	if ( hpFrame )
	{
		HPBar.animateSetpoint = stats[player.playernum]->HP;
		HPBar.animateValue = HPBar.animateSetpoint;
		HPBar.animateValue2 = HPBar.animateSetpoint;
		HPBar.animatePreviousSetpoint = HPBar.animateSetpoint;
		HPBar.animateState = ANIMATE_NONE;
	}
	if ( mpFrame )
	{
		MPBar.animateSetpoint = stats[player.playernum]->MP;
		MPBar.animateValue = MPBar.animateSetpoint;
		MPBar.animateValue2 = MPBar.animateSetpoint;
		MPBar.animatePreviousSetpoint = MPBar.animateSetpoint;
		MPBar.animateState = ANIMATE_NONE;
	}
}

void Player::HUD_t::updateXPBar()
{
	if ( !xpFrame )
	{
		return;
	}

	SDL_Rect pos = xpFrame->getSize();
	pos.x = hudFrame->getSize().w / 2 - pos.w / 2;
	pos.y = hudFrame->getSize().h - XP_FRAME_START_Y;
	xpFrame->setSize(pos);

	xpBar.animateSetpoint = std::min(100, stats[player.playernum]->EXP);
	xpBar.maxValue = 1000.0;

	if ( xpBar.animateState == ANIMATE_LEVELUP_RISING )
	{
		xpBar.animateTicks = ticks;

		real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		xpBar.animateValue += fpsScale * (10); // constant speed
		xpBar.animateValue = std::min(xpBar.maxValue, xpBar.animateValue);
		if ( xpBar.animateValue == xpBar.maxValue )
		{
			xpBar.animateState = ANIMATE_LEVELUP_FALLING;
		}
	}
	else if ( xpBar.animateState == ANIMATE_LEVELUP_FALLING )
	{
		if ( ticks - xpBar.animateTicks > TICKS_PER_SECOND * 2 )
		{
			int decrement = 40;
			double scaledDecrement = (decrement * (144.f / std::max(1U, fpsLimit)));
			xpBar.animateValue -= scaledDecrement;
			if ( xpBar.animateValue <= 0 )
			{
				xpBar.animateValue = 0.0;
				xpBar.xpLevelups = 0; // disable looping maybe one day use it
				if ( xpBar.xpLevelups > 1 )
				{
					--xpBar.xpLevelups;
					xpBar.animateState = ANIMATE_LEVELUP_RISING;
				}
				else
				{
					xpBar.xpLevelups = 0;
					xpBar.animateState = ANIMATE_NONE;
				}
			}
		}
	}
	else
	{
		int increment = 3;
		double scaledIncrement = (increment * (144.f / std::max(1U, fpsLimit)));
		if ( xpBar.animateValue < xpBar.animateSetpoint * 10 )
		{
			//real_t diff = std::max(.1, (xpBar.animateSetpoint * 10 - xpBar.animateValue) / 200.0); // 0.1-5 value
			//if ( xpBar.animateSetpoint * 10 >= xpBar.maxValue )
			//{
			//	diff = 5;
			//}
			//scaledIncrement *= 0.2 * pow(diff, 2) + .5;
			//xpBar.animateValue = std::min(xpBar.animateSetpoint * 10.0, xpBar.animateValue + scaledIncrement);

			real_t setpointDiff = std::max(50.0, xpBar.animateSetpoint * 10.0 - xpBar.animateValue);
			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
			xpBar.animateValue += fpsScale * (setpointDiff / 50.0); // reach it in x intervals, scaled to FPS
			xpBar.animateValue = std::min(static_cast<real_t>(xpBar.animateSetpoint * 10.0), xpBar.animateValue);
			//messagePlayer(0, "%.2f | %.2f", diff, scaledIncrement);
		}
		//else if ( xpBar.animateValue > xpBar.animateSetpoint * 10 )
		//{
		//	real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		//	xpBar.animateValue += fpsScale * (10); // constant speed
		//	xpBar.animateValue = std::min(xpBar.maxValue, xpBar.animateValue);
		//}
		//else
		//{
		//	xpBar.animateTicks = ticks;
		//}

		xpBar.animateTicks = ticks;

		//if ( xpBar.animateValue == xpBar.maxValue )
		//{
		//	xpBar.animateState = ANIMATE_LEVELUP;
		//}
	}

	char playerXPText[16];
	snprintf(playerXPText, sizeof(playerXPText), "%.f", xpBar.animateValue / 10);
	
	auto xpText = xpFrame->findField("xp text current");
	xpText->setText(playerXPText);

	auto xpBg = xpFrame->findImage("xp img base");
	auto xpProgress = xpFrame->findImage("xp img progress");
	auto xpProgressEndCap = xpFrame->findImage("xp img progress endcap");

	real_t percent = xpBar.animateValue / 1000.0;
	xpProgress->pos.w = std::max(1, static_cast<int>((xpBg->pos.w - xpProgressEndCap->pos.w) * percent));
	xpProgressEndCap->pos.x = xpProgress->pos.x + xpProgress->pos.w;
}

SDL_Surface* blitEnemyBar(const int player, SDL_Surface* statusEffectSprite)
{
	Frame* frame = players[player]->hud.enemyBarFrame;
	if ( !frame || !players[player]->isLocalPlayer() )
	{
		return nullptr;
	}

	auto baseBg = frame->findImage("base img");
	auto baseEndCap = frame->findImage("base img endcap");
	real_t frameOpacity = frame->getOpacity() / 100.0;

	auto foregroundFrame = frame->findFrame("bar progress frame");
	auto hpProgress = foregroundFrame->findImage("progress img");
	auto hpProgressEndcap = foregroundFrame->findImage("progress img endcap");

	auto dmgFrame = frame->findFrame("bar dmg frame");
	auto dmgProgress = dmgFrame->findImage("dmg img");
	auto dmgEndCap = dmgFrame->findImage("dmg img endcap");

	auto skullFrame = frame->findFrame("skull frame");
	int totalWidth = baseBg->pos.x + baseBg->pos.w + baseEndCap->pos.w;
	int totalHeight = frame->getSize().h;
	int statusEffectOffsetY = 0;
	if ( statusEffectSprite ) 
	{ 
		statusEffectOffsetY = statusEffectSprite->h;
		totalHeight += statusEffectOffsetY;
	}

	SDL_Surface* sprite = SDL_CreateRGBSurface(0, totalWidth, totalHeight, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	for ( auto& img : frame->getImages() )
	{
		SDL_Surface* srcSurf = const_cast<SDL_Surface*>(Image::get(img->path.c_str())->getSurf());
		Uint8 r, g, b, a;
		SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
		SDL_SetSurfaceAlphaMod(srcSurf, a * frameOpacity);
		SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
		SDL_Rect pos = img->pos;
		pos.y += statusEffectOffsetY;
		SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);
	}
	for ( auto& img : dmgFrame->getImages() )
	{
		SDL_Surface* srcSurf = const_cast<SDL_Surface*>(Image::get(img->path.c_str())->getSurf());
		Uint8 r, g, b, a;
		SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
		SDL_SetSurfaceAlphaMod(srcSurf, a * frameOpacity);
		//SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
		SDL_Rect pos = img->pos;
		pos.x += dmgFrame->getSize().x;
		pos.y += dmgFrame->getSize().y;
		pos.y += statusEffectOffsetY;
		SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);
	}
	for ( auto& img : foregroundFrame->getImages() )
	{
		SDL_Surface* srcSurf = const_cast<SDL_Surface*>(Image::get(img->path.c_str())->getSurf());
		Uint8 r, g, b, a;
		SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
		SDL_SetSurfaceAlphaMod(srcSurf, a * frameOpacity);
		//SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
		SDL_Rect pos = img->pos;
		pos.x += foregroundFrame->getSize().x;
		pos.y += foregroundFrame->getSize().y;
		pos.y += statusEffectOffsetY;
		SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);
	}
	for ( auto& img : skullFrame->getImages() )
	{
		SDL_Surface* srcSurf = const_cast<SDL_Surface*>(Image::get(img->path.c_str())->getSurf());
		Uint8 r, g, b, a;
		SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
		SDL_SetSurfaceAlphaMod(srcSurf, a * frameOpacity);
		SDL_Rect pos = img->pos;
		pos.x += skullFrame->getSize().x;
		pos.y += skullFrame->getSize().y;
		pos.y += statusEffectOffsetY;
		SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);
	}
	for ( auto& txt : frame->getFields() )
	{
		auto textGet = Text::get(txt->getText(), txt->getFont(),
			makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
		SDL_Surface* txtSurf = const_cast<SDL_Surface*>(textGet->getSurf());
		SDL_Rect pos;
		pos.w = textGet->getWidth();
		pos.h = textGet->getHeight();
		pos.x = sprite->w / 2 - pos.w / 2;
		pos.y = frame->getSize().h / 2 - pos.h / 2;
		pos.y += statusEffectOffsetY;
		Uint8 r, g, b, a;
		SDL_GetRGBA(txt->getColor(), mainsurface->format, &r, &g, &b, &a);
		SDL_SetSurfaceAlphaMod(txtSurf, a * frameOpacity);
		SDL_BlitSurface(txtSurf, nullptr, sprite, &pos);
	}
	if ( statusEffectSprite )
	{
		SDL_Rect pos{0, 0, statusEffectSprite->w, statusEffectSprite->h};
		SDL_BlitSurface(statusEffectSprite, nullptr, sprite, &pos);
	}
	return sprite;
}

SDL_Surface* EnemyHPDamageBarHandler::EnemyHPDetails::blitEnemyBarStatusEffects(const int player)
{
	Entity* entity = uidToEntity(enemy_uid);
	if ( entity && (entity->behavior != &actPlayer && entity->behavior != &actMonster) )
	{
		return nullptr;
	}
	Frame* frame = players[player]->hud.enemyBarFrame;
	if ( !frame || !players[player]->isLocalPlayer() )
	{
		return nullptr;
	}
	if ( enemy_statusEffects1 == 0 && enemy_statusEffects2 == 0 )
	{
		return nullptr;
	}
	auto baseBg = frame->findImage("base img");
	auto baseEndCap = frame->findImage("base img endcap");
	real_t frameOpacity = frame->getOpacity() / 100.0;
	const int maxWidth = baseBg->pos.x + baseBg->pos.w + baseEndCap->pos.w;
	const int iconHeight = 32;
	const int iconWidth = 32;

	SDL_Surface* sprite = SDL_CreateRGBSurface(0, maxWidth, iconHeight + 2, 32,
		0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

	int playernum = -1;
	if ( entity && entity->behavior == &actPlayer )
	{
		playernum = entity->skill[2];
	}

	int currentX = (maxWidth / 2);
	bool anyStatusEffect = false;

	std::vector<std::pair<SDL_Surface*, bool>> statusEffectIcons;
	if ( enemy_statusEffects1 != 0 )
	{
		for ( int i = 0; i < 32; ++i )
		{
			if ( (enemy_statusEffects1 & (1 << i)) != 0 )
			{
				if ( SDL_Surface* srcSurf = getStatusEffectSprite(entity,
					(entity ? entity->getStats() : nullptr), i, playernum) )
				{
					bool blinking = false;
					if ( (enemy_statusEffectsLowDuration1 & (1 << i)) != 0 )
					{
						blinking = true;
					}
					statusEffectIcons.push_back(std::make_pair(srcSurf, blinking));
				}
			}
		}
	}
	if ( enemy_statusEffects2 != 0 )
	{
		for ( int i = 0; i < 32; ++i )
		{
			if ( (enemy_statusEffects2 & (1 << i)) != 0 )
			{
				if ( SDL_Surface* srcSurf = getStatusEffectSprite(entity,
					(entity ? entity->getStats() : nullptr), i + 32, playernum) )
				{
					bool blinking = false;
					if ( (enemy_statusEffectsLowDuration2 & (1 << i)) != 0 )
					{
						blinking = true;
					}
					statusEffectIcons.push_back(std::make_pair(srcSurf, blinking));
				}
			}
		}
	}

	const int numIcons = statusEffectIcons.size();
	const int iconTotalWidth = iconWidth + 2;
	if ( numIcons % 2 == 1 ) // odd numbered
	{
		currentX -= ((iconTotalWidth) * (numIcons / 2)) + (iconTotalWidth / 2);
	}
	else
	{
		currentX -= ((iconTotalWidth) * (numIcons / 2));
	}

	for ( auto& icon : statusEffectIcons )
	{
		anyStatusEffect = true;
		int tickModifier = ticks % 25;
		real_t alpha = 255 * frameOpacity;
		if ( tickModifier >= 12 && icon.second )
		{
			alpha = 0;
		}
		SDL_SetSurfaceAlphaMod(icon.first, alpha);
		//SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
		SDL_Rect pos{ currentX, 0, iconWidth, iconHeight };
		SDL_BlitScaled(icon.first, nullptr, sprite, &pos);
		currentX += (iconWidth + 2);
	}

	if ( !anyStatusEffect )
	{
		SDL_FreeSurface(sprite);
		sprite = nullptr;
	}

	return sprite;
}

void Player::HUD_t::updateEnemyBar2(Frame* whichFrame, void* enemyHPDetails)
{
	if ( !whichFrame )
	{
		return;
	}

	if ( !enemyHPDetails )
	{
		return;
	}

	if ( !player.isLocalPlayer() )
	{
		return;
	}

	EnemyHPDamageBarHandler::EnemyHPDetails* enemyDetails = static_cast<EnemyHPDamageBarHandler::EnemyHPDetails*>(enemyHPDetails);

	Entity* entity = uidToEntity(enemyDetails->enemy_uid);
	if ( entity )
	{
		//enemyDetails->updateWorldCoordinates(); --moved to main loop before drawEntities3D
	}
	else
	{
		enemyDetails->enemy_hp = 0;
	}

	bool bIsMostRecentHPBar = enemyHPDamageBarHandler[player.playernum].getMostRecentHPBar() == enemyDetails;
	if ( bIsMostRecentHPBar && !enemyDetails->hasDistanceCheck )
	{
		//enemyDetails->hasDistanceCheck = true;
		auto& camera = cameras[player.playernum];
		double playerdist = sqrt(pow(camera.x * 16.0 - enemyDetails->worldX, 2) + pow(camera.y * 16.0 - enemyDetails->worldY, 2));
		if ( playerdist >= 3 * 16.0 )
		{
			//enemyDetails->displayOnHUD = true;
		}
	}


	SDL_Rect pos = whichFrame->getSize();
	bool doFadeout = false;
	bool doAnimation = true;
	if ( enemyDetails->displayOnHUD && whichFrame == enemyBarFrameHUD )
	{
		doAnimation = false;
	}

	if ( enemyDetails->expired == true )
	{
		doFadeout = true;
	}
	else
	{
		enemyDetails->animator.fadeOut = 100.0;
	}

	if ( doFadeout )
	{
		if ( doAnimation )
		{
			enemyDetails->animator.fadeOut -= 10.0 * (60.f / std::max(1U, fpsLimit));
			if ( enemyDetails->animator.fadeOut < 0.0 ) 
			{ 
				enemyDetails->animator.fadeOut = 0.0; 
				enemyDetails->animator.fadeIn = 0.0;
			}
		}
		whichFrame->setOpacity(enemyDetails->animator.fadeOut);
	}
	else
	{
		enemyDetails->animator.fadeIn = 100.0;
		whichFrame->setOpacity(enemyDetails->animator.fadeIn);
	}

	auto baseBg = whichFrame->findImage("base img");
	auto baseEndCap = whichFrame->findImage("base img endcap");

	auto foregroundFrame = whichFrame->findFrame("bar progress frame");
	auto hpProgress = foregroundFrame->findImage("progress img");
	auto hpProgressEndcap = foregroundFrame->findImage("progress img endcap");

	auto dmgFrame = whichFrame->findFrame("bar dmg frame");
	auto dmgProgress = dmgFrame->findImage("dmg img");
	auto dmgEndCap = dmgFrame->findImage("dmg img endcap");
	//auto bubblesImg = dmgFrame->findImage("img bubbles");

	real_t progressWidth = ENEMYBAR_BAR_WIDTH - hpProgressEndcap->pos.w;
	int backgroundWidth = ENEMYBAR_BAR_WIDTH - baseEndCap->pos.w;

	auto nameTxt = whichFrame->findField("enemy name txt");

	// handle bar size changing
	{
		std::vector<std::pair<real_t, int>> widthHealthBreakpoints;
		if ( enemyDetails->barType == EnemyHPDamageBarHandler::BAR_TYPE_CREATURE )
		{
			widthHealthBreakpoints = EnemyHPDamageBarHandler::widthHealthBreakpointsMonsters;
		}
		else
		{
			widthHealthBreakpoints = EnemyHPDamageBarHandler::widthHealthBreakpointsFurniture;
		}
		if ( widthHealthBreakpoints.empty() ) // width %, then HP value
		{
			// build some defaults
			widthHealthBreakpoints.push_back(std::make_pair(0.5, 10));
			widthHealthBreakpoints.push_back(std::make_pair(0.60, 20));
			widthHealthBreakpoints.push_back(std::make_pair(0.70, 50));
			widthHealthBreakpoints.push_back(std::make_pair(0.80, 100));
			widthHealthBreakpoints.push_back(std::make_pair(0.90, 250));
			widthHealthBreakpoints.push_back(std::make_pair(1.00, 1000));
		}

		enemyDetails->animator.widthMultiplier = 1.0;
		auto prevIt = widthHealthBreakpoints.end();
		bool foundBreakpoint = false;
		for ( auto it = widthHealthBreakpoints.begin(); it != widthHealthBreakpoints.end(); ++it )
		{
			int healthThreshold = (*it).second;
			real_t widthEntry = (*it).first;
			if ( (int)enemyDetails->animator.maxValue <= healthThreshold )
			{
				real_t width = 0.0;
				if ( prevIt != widthHealthBreakpoints.end() )
				{
					width = (*prevIt).first;
					// get linear scaled value between the breakponts
					width += (widthEntry - (*prevIt).first) * ((int)enemyDetails->animator.maxValue - (*prevIt).second) / ((*it).second - (*prevIt).second);
				}
				else
				{
					// set as minimum width
					width = widthEntry;
				}
				enemyDetails->animator.widthMultiplier = width;
				foundBreakpoint = true;
				break;
			}
			prevIt = it;
		}
		if ( !foundBreakpoint )
		{
			enemyDetails->animator.widthMultiplier = widthHealthBreakpoints[widthHealthBreakpoints.size() - 1].first;
		}

		real_t multiplier = enemyDetails->animator.widthMultiplier;

		int diff = static_cast<int>(std::max(0.0, progressWidth - progressWidth * multiplier)); // how many pixels the progress bar shrinks
		progressWidth *= multiplier; // scale the progress bars
		baseBg->pos.w = backgroundWidth - diff; // move the background bar by x pixels
		baseEndCap->pos.x = baseBg->pos.x + baseBg->pos.w; // move the background endcap by the new width

		hpProgress->pos.w = progressWidth;
		hpProgressEndcap->pos.x = hpProgress->pos.x + hpProgress->pos.w;

		pos.x = (hudFrame->getSize().w / 2) - ENEMYBAR_FRAME_WIDTH / 2 - 6;
		int widthChange = ((ENEMYBAR_BAR_WIDTH - hpProgressEndcap->pos.w) - progressWidth);
		pos.x += widthChange / 2;
		SDL_Rect textPos = nameTxt->getSize();
		textPos.w = ENEMYBAR_FRAME_WIDTH - widthChange;
		textPos.y = baseBg->pos.y;
		textPos.h = baseBg->pos.h;
		nameTxt->setSize(textPos);
	}

	int startY = hudFrame->getSize().h - ENEMYBAR_FRAME_START_Y - 100;
	if ( doFadeout )
	{
		pos.y = startY + (15 * (100.0 - enemyDetails->animator.fadeOut) / 100.0); // slide out downwards
	}
	else
	{
		pos.y = startY;
	}
	whichFrame->setSize(pos);

	enemyDetails->animator.previousSetpoint = enemyDetails->animator.setpoint;
	real_t& hpForegroundValue = enemyDetails->animator.foregroundValue;
	real_t& hpFadedValue = enemyDetails->animator.backgroundValue;

	if ( doAnimation )
	{
		enemyDetails->animator.setpoint = enemyDetails->enemy_hp;
		if ( enemyDetails->animator.setpoint < enemyDetails->animator.previousSetpoint ) // insta-change as losing health
		{
			hpForegroundValue = enemyDetails->animator.setpoint;
			enemyDetails->animator.animateTicks = ticks;
		}

		if ( enemyDetails->animator.maxValue > enemyDetails->enemy_maxhp )
		{
			hpFadedValue = enemyDetails->animator.setpoint; // resetting game etc, stop fade animation sticking out of frame
		}
		enemyDetails->animator.maxValue = enemyDetails->enemy_maxhp;


		if ( hpForegroundValue < enemyDetails->animator.setpoint ) // gaining HP, animate
		{
			real_t setpointDiff = std::max(0.01, enemyDetails->animator.setpoint - hpForegroundValue);
			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
			hpForegroundValue += fpsScale * (setpointDiff / 20.0); // reach it in 20 intervals, scaled to FPS
			hpForegroundValue = std::min(static_cast<real_t>(enemyDetails->animator.setpoint), hpForegroundValue);

			if ( abs(enemyDetails->animator.setpoint) - abs(hpForegroundValue) <= .05 )
			{
				hpForegroundValue = enemyDetails->animator.setpoint;
			}
		}
		else if ( hpForegroundValue > enemyDetails->animator.setpoint ) // losing HP, snap to value
		{
			hpForegroundValue = enemyDetails->animator.setpoint;
		}

		if ( hpFadedValue < enemyDetails->animator.setpoint )
		{
			hpFadedValue = hpForegroundValue;
			enemyDetails->animator.animateTicks = ticks;
		}
		else if ( hpFadedValue > enemyDetails->animator.setpoint )
		{
			if ( ticks - enemyDetails->animator.animateTicks > 30 || enemyDetails->animator.setpoint <= 0 ) // fall after x ticks
			{
				real_t setpointDiff = std::max(0.01, hpFadedValue - enemyDetails->animator.setpoint);
				real_t fpsScale = (144.f / std::max(1U, fpsLimit));
				real_t intervals = 10.0;
				if ( enemyDetails->animator.setpoint <= 0 )
				{
					intervals = 30.0; // dramatic
				}
				hpFadedValue -= fpsScale * (setpointDiff / intervals); // reach it in X intervals, scaled to FPS
				hpFadedValue = std::max(static_cast<real_t>(enemyDetails->animator.setpoint), hpFadedValue);
			}
		}
		else
		{
			enemyDetails->animator.animateTicks = ticks;
		}
	}

	auto skullFrame = whichFrame->findFrame("skull frame");
	std::vector<std::pair<Frame::image_t*, int>> allSkulls;
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 0 img"), 0));
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 25 img"), 25));
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 50 img"), 50));
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 100 img"), 75));
	real_t healthPercentage = 100 * enemyDetails->animator.setpoint / std::max(1.0, enemyDetails->animator.maxValue);
	int skullIndex = 0;
	for ( auto& skull : allSkulls )
	{
		if ( !skull.first ) { continue; }

		Uint8 r, g, b, a;
		SDL_GetRGBA(skull.first->color, mainsurface->format, &r, &g, &b, &a);
		real_t& skullOpacity = enemyDetails->animator.skullOpacities[skullIndex];

		if ( doAnimation )
		{
			if ( (int)healthPercentage < skull.second )
			{
				real_t opacityChange = 4 * (60.f / std::max(1U, fpsLimit)); // change independent of fps
				skullOpacity = std::max(0, (int)(skullOpacity - opacityChange));
			}
			else if ( (int)healthPercentage >= skull.second )
			{
				real_t opacityChange = 255;// *(144.f / std::max(1U, fpsLimit)); // change independent of fps
				skullOpacity = std::min(255, (int)(skullOpacity + opacityChange));
			}
		}
		a = skullOpacity;
		a *= enemyDetails->animator.fadeOut / 100.0;
		skull.first->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		++skullIndex;
	}

	nameTxt->setText(enemyDetails->enemy_name.c_str());

	real_t foregroundPercent = hpForegroundValue / enemyDetails->animator.maxValue;
	hpProgress->pos.w = std::max(1, static_cast<int>((progressWidth)* foregroundPercent));
	hpProgressEndcap->pos.x = hpProgress->pos.x + hpProgress->pos.w;

	real_t fadePercent = hpFadedValue / enemyDetails->animator.maxValue;
	dmgProgress->pos.w = std::max(1, static_cast<int>((progressWidth)* fadePercent));
	dmgEndCap->pos.x = dmgProgress->pos.x + dmgProgress->pos.w;
	if ( dmgProgress->pos.w == 1 && enemyDetails->animator.setpoint <= 0 )
	{
		dmgProgress->disabled = true;
		real_t& opacity = enemyDetails->animator.damageFrameOpacity;
		if ( doAnimation )
		{
			real_t opacityChange = .5 * (144.f / std::max(1U, fpsLimit)); // change by .05% independent of fps
			opacity = std::max(0.0, opacity - opacityChange);
		}
		dmgFrame->setOpacity(opacity);
		dmgFrame->setOpacity(dmgFrame->getOpacity() * enemyDetails->animator.fadeOut / 100.0);
		// make this element fade out to the left
		dmgEndCap->pos.x = 0 - (dmgEndCap->pos.w * (1.0 - opacity / 100.0));
	}
	else
	{
		dmgProgress->disabled = false;
		enemyDetails->animator.damageFrameOpacity = enemyDetails->animator.fadeOut;
		dmgFrame->setOpacity(enemyDetails->animator.fadeOut);
	}

	if ( enemyDetails->animator.setpoint <= 0 )
	{
		// hide all the progress elements when dead, as endcap/base don't shrink
		// hpProgress width 0px defaults to original size, so hide that too
		hpProgress->disabled = true;
		hpProgressEndcap->disabled = true;
	}
	else
	{
		hpProgress->disabled = false;
		hpProgressEndcap->disabled = false;
	}

	// damage number on HUD
	if ( enemyDetails->displayOnHUD && whichFrame == enemyBarFrameHUD )
	{
		auto dmgText = hudFrame->findField("enemy dmg txt");
		Sint32 damageNumber = enemyDetails->animator.damageTaken;
		enemyDetails->animator.damageTaken = -1;
		if ( damageNumber >= 0 )
		{
			char buf[128] = "";
			snprintf(buf, sizeof(buf), "%d", damageNumber);
			dmgText->setText(buf);
			dmgText->setDisabled(false);
			SDL_Rect txtPos = dmgText->getSize();
			SDL_Rect barPos = foregroundFrame->getAbsoluteSize();
			//txtPos.x = barPos.x + baseBg->pos.x/* + baseBg->pos.w*/;
			//txtPos.y = barPos.y - 30;
			auto text = Text::get(dmgText->getText(), dmgText->getFont(),
				makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
			txtPos.x = 30 + player.camera_virtualWidth() / 2;
			txtPos.y = -50 + player.camera_virtualHeight() / 2;
			txtPos.w = text->getWidth();
			txtPos.h = text->getHeight();
			dmgText->setSize(txtPos);
			hudDamageTextVelocityX = 1.0;
			hudDamageTextVelocityY = 3.0;
			Uint8 r, g, b, a;
			SDL_GetRGBA(dmgText->getColor(), mainsurface->format, &r, &g, &b, &a);
			dmgText->setColor(makeColor(r, g, b, 255));
		}

		if ( !dmgText->isDisabled() )
		{
			SDL_Rect txtPos = dmgText->getSize();
			txtPos.x += hudDamageTextVelocityX;
			txtPos.y -= hudDamageTextVelocityY;
			hudDamageTextVelocityX += .05;
			hudDamageTextVelocityY -= .3;
			dmgText->setSize(txtPos);
			if ( hudDamageTextVelocityY < -2.0 )
			{
				Uint8 r, g, b, a;
				SDL_GetRGBA(dmgText->getColor(), mainsurface->format, &r, &g, &b, &a);
				a = (std::max(0, (int)a - 16));
				dmgText->setColor(makeColor(r, g, b, a));
				if ( a == 0 )
				{
					dmgText->setDisabled(true);
				}
			}
		}
	}

	if ( enemyDetails->worldTexture )
	{
		delete enemyDetails->worldTexture;
		enemyDetails->worldTexture = nullptr;
	}
	if ( enemyDetails->worldSurfaceSprite )
	{
		SDL_FreeSurface(enemyDetails->worldSurfaceSprite);
		enemyDetails->worldSurfaceSprite = nullptr;
	}
	if ( enemyDetails->worldSurfaceSpriteStatusEffects )
	{
		SDL_FreeSurface(enemyDetails->worldSurfaceSpriteStatusEffects);
		enemyDetails->worldSurfaceSpriteStatusEffects = nullptr;
	}

	enemyDetails->worldSurfaceSpriteStatusEffects = enemyDetails->blitEnemyBarStatusEffects(player.playernum);
	enemyDetails->worldSurfaceSprite = blitEnemyBar(player.playernum, enemyDetails->worldSurfaceSpriteStatusEffects);
	enemyDetails->worldTexture = new TempTexture();
	enemyDetails->worldTexture->load(enemyDetails->worldSurfaceSprite, false, true);

	whichFrame->setDisabled(true);
	if ( !enemyDetails->displayOnHUD )
	{
		//printTextFormatted(font16x16_bmp, 8, 8 + 4 * 16, "Any vertex visible: %d", anyVertexVisible);
	}
	else
	{
		if ( whichFrame == enemyBarFrameHUD )
		{
			whichFrame->setDisabled(false);
		}
		else
		{
			updateEnemyBar2(enemyBarFrameHUD, enemyHPDetails);
		}
	}
}

void Player::HUD_t::updateEnemyBar(Frame* whichFrame)
{
	if ( !whichFrame )
	{
		return;
	}

	if ( !player.isLocalPlayer() )
	{
		return;
	}

	SDL_Rect pos = whichFrame->getSize();
	bool doFadeout = false;

	EnemyHPDamageBarHandler::EnemyHPDetails* enemyDetails = nullptr;
	Bar_t* enemyBar = nullptr;
	Sint32 damageNumber = -1;
	enemyDetails = enemyHPDamageBarHandler[player.playernum].getMostRecentHPBar();
	enemyBar = &this->enemyBar;

	if ( enemyDetails )
	{
		enemyBar->animatePreviousSetpoint = enemyDetails->animator.previousSetpoint;
		enemyBar->animateValue = enemyDetails->animator.foregroundValue;
		enemyBar->animateValue2 = enemyDetails->animator.backgroundValue;
		enemyBar->animateSetpoint = enemyDetails->animator.setpoint;
		enemyBar->animateTicks = enemyDetails->animator.animateTicks;
		enemyBar->maxValue = enemyDetails->animator.maxValue;
		enemyBar->fadeOut = 100.0;
		damageNumber = enemyDetails->animator.damageTaken;
		enemyDetails->animator.damageTaken = -1;

		if ( Entity* entity = uidToEntity(enemyDetails->enemy_uid) )
		{
			enemyDetails->worldX = entity->x;
			enemyDetails->worldY = entity->y;
			enemyDetails->worldZ = entity->z;
		}
	}
	if ( !enemyDetails || enemyDetails->expired == true )
	{
		doFadeout = true;
	}

	if ( doFadeout )
	{
		enemyBar->fadeOut -= 10.0 * (60.f / std::max(1U, fpsLimit));
		if ( enemyBar->fadeOut < 0.0 ) { enemyBar->fadeOut = 0.0; enemyBar->fadeIn = 0.0; }
		whichFrame->setOpacity(enemyBar->fadeOut);
	}
	else
	{
		enemyBar->fadeIn = 100.0;
		whichFrame->setOpacity(enemyBar->fadeIn);
	}

	auto baseBg = whichFrame->findImage("base img");
	auto baseEndCap = whichFrame->findImage("base img endcap");
	
	auto foregroundFrame = whichFrame->findFrame("bar progress frame");
	auto hpProgress = foregroundFrame->findImage("progress img");
	auto hpProgressEndcap = foregroundFrame->findImage("progress img endcap");

	auto dmgFrame = whichFrame->findFrame("bar dmg frame");
	auto dmgProgress = dmgFrame->findImage("dmg img");
	auto dmgEndCap = dmgFrame->findImage("dmg img endcap");
	//auto bubblesImg = dmgFrame->findImage("img bubbles");

	real_t progressWidth = ENEMYBAR_BAR_WIDTH - hpProgressEndcap->pos.w;
	int backgroundWidth = ENEMYBAR_BAR_WIDTH - baseEndCap->pos.w;

	auto nameTxt = whichFrame->findField("enemy name txt");

	// handle bar size changing
	{
		std::vector<std::pair<real_t, int>>widthHealthBreakpoints; // width %, then HP value
		widthHealthBreakpoints.push_back(std::make_pair(0.5, 10));
		widthHealthBreakpoints.push_back(std::make_pair(0.60, 20));
		widthHealthBreakpoints.push_back(std::make_pair(0.70, 50));
		widthHealthBreakpoints.push_back(std::make_pair(0.80, 100));
		widthHealthBreakpoints.push_back(std::make_pair(0.90, 250));
		widthHealthBreakpoints.push_back(std::make_pair(1.00, 1000));

		enemyBar->widthMultiplier = 1.0;
		auto prevIt = widthHealthBreakpoints.end();
		bool foundBreakpoint = false;
		for ( auto it = widthHealthBreakpoints.begin(); it != widthHealthBreakpoints.end(); ++it )
		{
			int healthThreshold = (*it).second;
			real_t widthEntry = (*it).first;
			if ( (int)enemyBar->maxValue <= healthThreshold )
			{
				real_t width = 0.0;
				if ( prevIt != widthHealthBreakpoints.end() )
				{
					width = (*prevIt).first;
					// get linear scaled value between the breakponts
					width += (widthEntry - (*prevIt).first) * ((int)enemyBar->maxValue - (*prevIt).second) / ((*it).second - (*prevIt).second);
				}
				else
				{
					// set as minimum width
					width = widthEntry;  /**((int)enemyBar->maxValue) / ((*it).second);*/
				}
				enemyBar->widthMultiplier = width;
				foundBreakpoint = true;
				break;
			}
			prevIt = it;
		}
		if ( !foundBreakpoint )
		{
			enemyBar->widthMultiplier = widthHealthBreakpoints[widthHealthBreakpoints.size() - 1].first;
		}

		real_t multiplier = enemyBar->widthMultiplier;

		int diff = static_cast<int>(std::max(0.0, progressWidth - progressWidth * multiplier)); // how many pixels the progress bar shrinks
		progressWidth *= multiplier; // scale the progress bars
		baseBg->pos.w = backgroundWidth - diff; // move the background bar by x pixels
		baseEndCap->pos.x = baseBg->pos.x + baseBg->pos.w; // move the background endcap by the new width

		hpProgress->pos.w = progressWidth;
		hpProgressEndcap->pos.x = hpProgress->pos.x + hpProgress->pos.w;

		pos.x = (hudFrame->getSize().w / 2) - ENEMYBAR_FRAME_WIDTH / 2 - 6;
		int widthChange = ((ENEMYBAR_BAR_WIDTH - hpProgressEndcap->pos.w) - progressWidth);
		pos.x += widthChange / 2;
		SDL_Rect textPos = nameTxt->getSize();
		textPos.w = ENEMYBAR_FRAME_WIDTH - widthChange;
		textPos.y = baseBg->pos.y;
		textPos.h = baseBg->pos.h;
		nameTxt->setSize(textPos);
	}

	int startY = hudFrame->getSize().h - ENEMYBAR_FRAME_START_Y - 100;
	if ( doFadeout )
	{
		pos.y = startY + (15 * (100.0 - enemyBar->fadeOut) / 100.0); // slide out downwards
	}
	else
	{
		pos.y = startY;
	}
	whichFrame->setSize(pos);
	
	//messagePlayer(0, "%.2f | %.2f | %.2f | %d", enemyDetails->animateValue, 
	//	enemyDetails->animateValue2, enemyDetails->animatePreviousSetpoint, enemyDetails->animateSetpoint);

	enemyBar->animatePreviousSetpoint = enemyBar->animateSetpoint;
	real_t& hpForegroundValue = enemyBar->animateValue;
	real_t& hpFadedValue = enemyBar->animateValue2;

	if ( enemyDetails )
	{
		enemyBar->animateSetpoint = enemyDetails->enemy_hp;
		if ( enemyBar->animateSetpoint < enemyBar->animatePreviousSetpoint ) // insta-change as losing health
		{
			hpForegroundValue = enemyBar->animateSetpoint;
			enemyBar->animateTicks = ticks;
		}
	
		if ( enemyBar->maxValue > enemyDetails->enemy_maxhp )
		{
			hpFadedValue = enemyBar->animateSetpoint; // resetting game etc, stop fade animation sticking out of frame
		}
		enemyBar->maxValue = enemyDetails->enemy_maxhp;
	}


	if ( hpForegroundValue < enemyBar->animateSetpoint ) // gaining HP, animate
	{
		real_t setpointDiff = std::max(0.01, enemyBar->animateSetpoint - hpForegroundValue);
		real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		hpForegroundValue += fpsScale * (setpointDiff / 20.0); // reach it in 20 intervals, scaled to FPS
		hpForegroundValue = std::min(static_cast<real_t>(enemyBar->animateSetpoint), hpForegroundValue);

		if ( abs(enemyBar->animateSetpoint) - abs(hpForegroundValue) <= .05 )
		{
			hpForegroundValue = enemyBar->animateSetpoint;
		}
	}
	else if ( hpForegroundValue > enemyBar->animateSetpoint ) // losing HP, snap to value
	{
		hpForegroundValue = enemyBar->animateSetpoint;
	}

	if ( hpFadedValue < enemyBar->animateSetpoint )
	{
		hpFadedValue = hpForegroundValue;
		enemyBar->animateTicks = ticks;
	}
	else if ( hpFadedValue > enemyBar->animateSetpoint )
	{
		if ( ticks - enemyBar->animateTicks > 30 || enemyBar->animateSetpoint <= 0 ) // fall after x ticks
		{
			real_t setpointDiff = std::max(0.01, hpFadedValue - enemyBar->animateSetpoint);
			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
			real_t intervals = 10.0;
			if ( enemyBar->animateSetpoint <= 0 )
			{
				intervals = 30.0; // dramatic
			}
			hpFadedValue -= fpsScale * (setpointDiff / intervals); // reach it in X intervals, scaled to FPS
			hpFadedValue = std::max(static_cast<real_t>(enemyBar->animateSetpoint), hpFadedValue);
		}
	}
	else
	{
		enemyBar->animateTicks = ticks;
	}

	auto skullFrame = whichFrame->findFrame("skull frame");
	std::vector<std::pair<Frame::image_t*, int>> allSkulls;
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 0 img"), 0));
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 25 img"), 25));
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 50 img"), 50));
	allSkulls.push_back(std::make_pair(skullFrame->findImage("skull 100 img"), 75));
	real_t healthPercentage = 100 * enemyBar->animateSetpoint / std::max(1.0, enemyBar->maxValue);
	for ( auto& skull : allSkulls )
	{
		if ( !skull.first ) { continue; }

		Uint8 r, g, b, a;
		SDL_GetRGBA(skull.first->color, mainsurface->format, &r, &g, &b, &a);

		if ( (int)healthPercentage < skull.second )
		{
			real_t opacityChange = 4 * (60.f / std::max(1U, fpsLimit)); // change independent of fps
			a = std::max(0, a - (int)opacityChange);
		}
		else if ( (int)healthPercentage >= skull.second )
		{
			real_t opacityChange = 255;// *(144.f / std::max(1U, fpsLimit)); // change independent of fps
			a = std::min(255, a + (int)opacityChange);
		}
		a *= whichFrame->getOpacity() / 100.0;
		skull.first->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
	}

	//char playerHPText[16];
	//snprintf(playerHPText, sizeof(playerHPText), "%d", stats[player.playernum]->HP);
	if ( enemyDetails )
	{
		nameTxt->setText(enemyDetails->enemy_name.c_str());
	}

	//auto hpText = hpForegroundFrame->findField("hp text");
	//hpText->setText(playerHPText);

	real_t foregroundPercent = hpForegroundValue / enemyBar->maxValue;
	hpProgress->pos.w = std::max(1, static_cast<int>((progressWidth) * foregroundPercent));
	hpProgressEndcap->pos.x = hpProgress->pos.x + hpProgress->pos.w;

	real_t fadePercent = hpFadedValue / enemyBar->maxValue;
	dmgProgress->pos.w = std::max(1, static_cast<int>((progressWidth) * fadePercent));
	dmgEndCap->pos.x = dmgProgress->pos.x + dmgProgress->pos.w;
	if ( dmgProgress->pos.w == 1 && enemyBar->animateSetpoint <= 0 )
	{
		dmgProgress->disabled = true;
		real_t opacity = dmgFrame->getOpacity();
		real_t opacityChange = .5 * (144.f / std::max(1U, fpsLimit)); // change by .05% independent of fps
		dmgFrame->setOpacity(std::max(0.0, opacity - opacityChange));
		dmgFrame->setOpacity(dmgFrame->getOpacity() * whichFrame->getOpacity() / 100.0);
		// make this element fade out to the left
		dmgEndCap->pos.x = 0 - (dmgEndCap->pos.w * (1.0 - opacity / 100.0));
	}
	else
	{
		dmgProgress->disabled = false;
		dmgFrame->setOpacity(whichFrame->getOpacity());
	}

	//bubblesImg->pos.x = dmgEndCap->pos.x - bubblesImg->pos.w;

	if ( enemyBar->animateSetpoint <= 0 )
	{
		// hide all the progress elements when dead, as endcap/base don't shrink
		// hpProgress width 0px defaults to original size, so hide that too
		hpProgress->disabled = true;
		hpProgressEndcap->disabled = true;
	}
	else
	{
		hpProgress->disabled = false;
		hpProgressEndcap->disabled = false;
	}

	auto dmgText = hudFrame->findField("enemy dmg txt");
	// damage number on HUD
	{
		if ( damageNumber >= 0 )
		{
			char buf[128] = "";
			snprintf(buf, sizeof(buf), "%d", damageNumber);
			dmgText->setText(buf);
			dmgText->setDisabled(false);
			SDL_Rect txtPos = dmgText->getSize();
			SDL_Rect barPos = foregroundFrame->getAbsoluteSize();
			//txtPos.x = barPos.x + baseBg->pos.x/* + baseBg->pos.w*/;
			//txtPos.y = barPos.y - 30;
			auto text = Text::get(dmgText->getText(), dmgText->getFont(),
				makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));
			txtPos.x = 30 + player.camera_virtualWidth() / 2;
			txtPos.y = -50 + player.camera_virtualHeight() / 2;
			txtPos.w = text->getWidth();
			txtPos.h = text->getHeight();
			dmgText->setSize(txtPos);
			hudDamageTextVelocityX = 1.0;
			hudDamageTextVelocityY = 3.0;
			Uint8 r, g, b, a;
			SDL_GetRGBA(dmgText->getColor(), mainsurface->format, &r, &g, &b, &a);
			dmgText->setColor(makeColor(r, g, b, 255));
		}

		if ( !dmgText->isDisabled() )
		{
			SDL_Rect txtPos = dmgText->getSize();
			txtPos.x += hudDamageTextVelocityX;
			txtPos.y -= hudDamageTextVelocityY;
			hudDamageTextVelocityX += .05;
			hudDamageTextVelocityY -= .3;
			dmgText->setSize(txtPos);
			if ( hudDamageTextVelocityY < -2.0 )
			{
				Uint8 r, g, b, a;
				SDL_GetRGBA(dmgText->getColor(), mainsurface->format, &r, &g, &b, &a);
				a = (std::max(0, (int)a - 16));
				dmgText->setColor(makeColor(r, g, b, a));
				if ( a == 0 )
				{
					dmgText->setDisabled(true);
				}
			}
		}
	}

	if ( enemyDetails )
	{
		enemyDetails->animator.previousSetpoint = enemyBar->animatePreviousSetpoint;
		enemyDetails->animator.foregroundValue = enemyBar->animateValue;
		enemyDetails->animator.backgroundValue = enemyBar->animateValue2;
		enemyDetails->animator.setpoint = enemyBar->animateSetpoint;
		enemyDetails->animator.animateTicks = enemyBar->animateTicks;
		enemyDetails->animator.maxValue = enemyBar->maxValue;
		enemyDetails->animator.fadeIn = enemyBar->fadeIn;
		enemyDetails->animator.fadeOut = enemyBar->fadeOut;
		if ( enemyDetails->worldTexture )
		{
			delete enemyDetails->worldTexture;
			enemyDetails->worldTexture = nullptr;
		}
		if ( enemyDetails->worldSurfaceSprite )
		{
			SDL_FreeSurface(enemyDetails->worldSurfaceSprite);
			enemyDetails->worldSurfaceSprite = nullptr;
		}
		enemyDetails->worldSurfaceSprite = blitEnemyBar(player.playernum, enemyDetails->worldSurfaceSpriteStatusEffects);
		enemyDetails->worldTexture = new TempTexture();
		enemyDetails->worldTexture->load(enemyDetails->worldSurfaceSprite, false, true);
	}

	whichFrame->setDisabled(true);
}

void Player::HUD_t::updateHPBar()
{
	if ( !hpFrame )
	{
		return;
	}

	SDL_Rect pos = hpFrame->getSize();
	pos.x = HPMP_FRAME_START_X;
	pos.y = hudFrame->getSize().h - HPMP_FRAME_START_Y;
	hpFrame->setSize(pos);

	auto hpForegroundFrame = hpFrame->findFrame("hp foreground frame");
	auto hpBg = hpFrame->findImage("hp img base");
	auto hpEndcap = hpForegroundFrame->findImage("hp img endcap");
	auto hpProgressBot = hpForegroundFrame->findImage("hp img progress bot");
	auto hpProgress = hpForegroundFrame->findImage("hp img progress");
	auto hpProgressEndCap = hpForegroundFrame->findImage("hp img progress endcap");
	auto hpFadeFrame = hpFrame->findFrame("hp fade frame");
	auto hpFadedBase = hpFadeFrame->findImage("hp img fade bot");
	auto hpFaded = hpFadeFrame->findImage("hp img fade");
	auto hpFadedEndCap = hpFadeFrame->findImage("hp img fade endcap");

	real_t progressWidth = hpFrame->getSize().w - 74;
	int backgroundWidth = hpFrame->getSize().w - 54;

	// handle bar size changing
	{
		real_t multiplier = 1.0;
		const Sint32 maxHPWidth = 160;
		if ( stats[player.playernum]->MAXHP < maxHPWidth )
		{
			// start at 30%, increase 2.5% every 5 HP past 20 MAXHP
			multiplier = .3 + (.025 * ((std::max(0, stats[player.playernum]->MAXHP - 20) / 5)));
		}

		int diff = static_cast<int>(std::max(0.0, progressWidth - progressWidth * multiplier)); // how many pixels the progress bar shrinks
		progressWidth *= multiplier; // scale the progress bars
		hpBg->pos.w = backgroundWidth - diff; // move the background bar by x pixels as above
		hpEndcap->pos.x = hpFrame->getSize().w - hpEndcap->pos.w - diff; // move the background endcap by x pixels as above
	}

	HPBar.animatePreviousSetpoint = HPBar.animateSetpoint;
	real_t& hpForegroundValue = HPBar.animateValue;
	real_t& hpFadedValue = HPBar.animateValue2;

	HPBar.animateSetpoint = stats[player.playernum]->HP;
	if ( HPBar.animateSetpoint < HPBar.animatePreviousSetpoint ) // insta-change as losing health
	{
		hpForegroundValue = HPBar.animateSetpoint;
		HPBar.animateTicks = ticks;
	}

	if ( HPBar.maxValue > stats[player.playernum]->MAXHP )
	{
		hpFadedValue = HPBar.animateSetpoint; // resetting game etc, stop fade animation sticking out of frame
	}

	HPBar.maxValue = stats[player.playernum]->MAXHP;

	if ( hpForegroundValue < HPBar.animateSetpoint ) // gaining HP, animate
	{
		real_t setpointDiff = std::max(0.0, HPBar.animateSetpoint - hpForegroundValue);
		real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		hpForegroundValue += fpsScale * (setpointDiff / 20.0); // reach it in 20 intervals, scaled to FPS
		hpForegroundValue = std::min(static_cast<real_t>(HPBar.animateSetpoint), hpForegroundValue);

		if ( abs(HPBar.animateSetpoint) - abs(hpForegroundValue) <= 1.0 )
		{
			hpForegroundValue = HPBar.animateSetpoint;
		}

		/*	int increment = 3;
			double scaledIncrement = (increment * (144.f / std::max(1U, fpsLimit)));*/
		//real_t diff = std::max(.1, (HPBar.animateSetpoint * 10 - hpForegroundValue) / (maxValue / 5)); // 0.1-5 value
		//if ( HPBar.animateSetpoint * 10 >= maxValue )
		//{
		//	diff = 5;
		//}
		//scaledIncrement *= 0.2 * pow(diff, 2) + .5;

		//hpForegroundValue = std::min(HPBar.animateSetpoint * 10.0, hpForegroundValue + scaledIncrement);
		//messagePlayer(0, "%.2f | %.2f", hpForegroundValue);
	}
	else if ( hpForegroundValue > HPBar.animateSetpoint ) // losing HP, snap to value
	{
		hpForegroundValue = HPBar.animateSetpoint;
	}

	if ( hpFadedValue < HPBar.animateSetpoint )
	{
		hpFadedValue = hpForegroundValue;
		HPBar.animateTicks = ticks;
	}
	else if ( hpFadedValue > HPBar.animateSetpoint )
	{
		if ( ticks - HPBar.animateTicks > 30 /*|| stats[player.playernum]->HP <= 0*/ ) // fall after x ticks
		{
			real_t setpointDiff = std::max(0.01, hpFadedValue - HPBar.animateSetpoint);
			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
			hpFadedValue -= fpsScale * (setpointDiff / 20.0); // reach it in 20 intervals, scaled to FPS
			hpFadedValue = std::max(static_cast<real_t>(HPBar.animateSetpoint), hpFadedValue);
		}
	}
	else
	{
		HPBar.animateTicks = ticks;
	}

	char playerHPText[16];
	snprintf(playerHPText, sizeof(playerHPText), "%d", stats[player.playernum]->HP);

	auto hpText = hpForegroundFrame->findField("hp text");
	hpText->setText(playerHPText);

	real_t foregroundPercent = hpForegroundValue / HPBar.maxValue;
	hpProgress->pos.w = std::max(1, static_cast<int>((progressWidth) * foregroundPercent));
	hpProgressEndCap->pos.x = hpProgress->pos.x + hpProgress->pos.w;

	real_t fadePercent = hpFadedValue / HPBar.maxValue;
	hpFaded->pos.w = std::max(1, static_cast<int>((progressWidth) * fadePercent));
	hpFadedEndCap->pos.x = hpFaded->pos.x + hpFaded->pos.w;
	if ( hpFaded->pos.w == 1 && stats[player.playernum]->HP <= 0 )
	{
		hpFaded->disabled = true;
		real_t opacity = hpFadeFrame->getOpacity();
		real_t opacityChange = .5 * (144.f / std::max(1U, fpsLimit)); // change by .05% independant of fps
		hpFadeFrame->setOpacity(std::max(0.0, opacity - opacityChange));

		// make this element fade out to the left, starting 54px then finally at 40px. @ 40px it's out of shot (6 width + 8 endcap width)
		hpFadedBase->pos.x = 54 - (14 * (1.0 - opacity / 100.0)); 
		hpFadedEndCap->pos.x = hpFadedBase->pos.x + hpFadedBase->pos.w;
	}
	else
	{
		hpFaded->disabled = false;
		hpFadeFrame->setOpacity(100.0);

		// reset to 54px left as we altered in above if statement
		hpFadedBase->pos.x = 54;
	}

	if ( stats[player.playernum]->HP <= 0 ) 
	{
		// hide all the progress elements when dead, as endcap/base don't shrink
		// hpProgress width 0px defaults to original size, so hide that too
		hpProgress->disabled = true;
		hpProgressEndCap->disabled = true;
		hpProgressBot->disabled = true;
	}
	else
	{
		hpProgress->disabled = false;
		hpProgressEndCap->disabled = false;
		hpProgressBot->disabled = false;
	}
}

void Player::HUD_t::updateMPBar()
{
	if ( !mpFrame )
	{
		return;
	}

	SDL_Rect pos = mpFrame->getSize();
	pos.x = HPMP_FRAME_START_X;
	pos.y = hudFrame->getSize().h - HPMP_FRAME_START_Y + HPMP_FRAME_HEIGHT;
	mpFrame->setSize(pos);

	auto mpForegroundFrame = mpFrame->findFrame("mp foreground frame");
	auto mpBg = mpFrame->findImage("mp img base");
	auto mpEndcap = mpForegroundFrame->findImage("mp img endcap");
	auto mpProgressBot = mpForegroundFrame->findImage("mp img progress bot");
	auto mpProgress = mpForegroundFrame->findImage("mp img progress");
	auto mpProgressEndCap = mpForegroundFrame->findImage("mp img progress endcap");
	auto mpFadeFrame = mpFrame->findFrame("mp fade frame");
	auto mpFadedBase = mpFadeFrame->findImage("mp img fade bot");
	auto mpFaded = mpFadeFrame->findImage("mp img fade");
	auto mpFadedEndCap = mpFadeFrame->findImage("mp img fade endcap");

	real_t progressWidth = mpFrame->getSize().w - 74;
	int backgroundWidth = mpFrame->getSize().w - 54;

	// handle bar size changing
	{
		real_t multiplier = 1.0;
		const Sint32 maxMPWidth = 160;
		if ( stats[player.playernum]->MAXMP < maxMPWidth )
		{
			// start at 30%, increase 2.5% every 5 MP past 20 MAXMP
			multiplier = .3 + (.025 * ((std::max(0, stats[player.playernum]->MAXMP - 20) / 5)));
		}

		int diff = static_cast<int>(std::max(0.0, progressWidth - progressWidth * multiplier)); // how many pixels the progress bar shrinks
		progressWidth *= multiplier; // scale the progress bars
		mpBg->pos.w = backgroundWidth - diff; // move the background bar by x pixels as above
		mpEndcap->pos.x = mpFrame->getSize().w - mpEndcap->pos.w - diff; // move the background endcap by x pixels as above
	}

	MPBar.animatePreviousSetpoint = MPBar.animateSetpoint;
	real_t& mpForegroundValue = MPBar.animateValue;
	real_t& mpFadedValue = MPBar.animateValue2;

	MPBar.animateSetpoint = stats[player.playernum]->MP;
	if ( MPBar.animateSetpoint < MPBar.animatePreviousSetpoint ) // insta-change as losing health
	{
		mpForegroundValue = MPBar.animateSetpoint;
		MPBar.animateTicks = ticks;
	}

	if ( MPBar.maxValue > stats[player.playernum]->MAXMP )
	{
		mpFadedValue = MPBar.animateSetpoint; // resetting game etc, stop fade animation sticking out of frame
	}

	MPBar.maxValue = stats[player.playernum]->MAXMP;
	if ( mpForegroundValue < MPBar.animateSetpoint ) // gaining MP, animate
	{
		real_t setpointDiff = std::max(.1, MPBar.animateSetpoint - mpForegroundValue);
		real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		mpForegroundValue += fpsScale * (setpointDiff / 20.0); // reach it in 20 intervals, scaled to FPS
		mpForegroundValue = std::min(static_cast<real_t>(MPBar.animateSetpoint), mpForegroundValue);

		/*if ( abs(MPBar.animateSetpoint) - abs(mpForegroundValue) <= 1.0 )
		{
			mpForegroundValue = MPBar.animateSetpoint;
		}*/
	}
	else if ( mpForegroundValue > MPBar.animateSetpoint ) // losing MP, snap to value
	{
		mpForegroundValue = MPBar.animateSetpoint;
	}

	if ( mpFadedValue < MPBar.animateSetpoint )
	{
		mpFadedValue = mpForegroundValue;
		MPBar.animateTicks = ticks;
	}
	else if ( mpFadedValue > MPBar.animateSetpoint )
	{
		if ( ticks - MPBar.animateTicks > 30 /*|| stats[player.playernum]->MP <= 0*/ ) // fall after x ticks
		{
			real_t setpointDiff = std::max(0.1, mpFadedValue - MPBar.animateSetpoint);
			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
			mpFadedValue -= fpsScale * (setpointDiff / 20.0); // reach it in 20 intervals, scaled to FPS
			mpFadedValue = std::max(static_cast<real_t>(MPBar.animateSetpoint), mpFadedValue);
		}
	}
	else
	{
		MPBar.animateTicks = ticks;
	}

	char playerMPText[16];
	snprintf(playerMPText, sizeof(playerMPText), "%d", stats[player.playernum]->MP);

	auto mpText = mpForegroundFrame->findField("mp text");
	mpText->setText(playerMPText);

	real_t foregroundPercent = mpForegroundValue / MPBar.maxValue;
	mpProgress->pos.w = std::max(1, static_cast<int>((progressWidth)* foregroundPercent));
	mpProgressEndCap->pos.x = mpProgress->pos.x + mpProgress->pos.w;

	real_t fadePercent = mpFadedValue / MPBar.maxValue;
	mpFaded->pos.w = std::max(1, static_cast<int>((progressWidth)* fadePercent));
	mpFadedEndCap->pos.x = mpFaded->pos.x + mpFaded->pos.w;
	if ( mpFaded->pos.w == 1 && stats[player.playernum]->MP <= 0 )
	{
		mpFaded->disabled = true;
		real_t opacity = mpFadeFrame->getOpacity();
		real_t opacityChange = .5 * (144.f / std::max(1U, fpsLimit)); // change by .05% independant of fps
		mpFadeFrame->setOpacity(std::max(0.0, opacity - opacityChange));

		// make this element fade out to the left, starting 54px then finally at 40px. @ 40px it's out of shot (6 width + 8 endcap width)
		mpFadedBase->pos.x = 54 - (14 * (1.0 - opacity / 100.0));
		mpFadedEndCap->pos.x = mpFadedBase->pos.x + mpFadedBase->pos.w;
	}
	else
	{
		mpFaded->disabled = false;
		mpFadeFrame->setOpacity(100.0);

		// reset to 54px left as we altered in above if statement
		mpFadedBase->pos.x = 54;
	}

	if ( stats[player.playernum]->MP <= 0 )
	{
		// hide all the progress elements when dead, as endcap/base don't shrink
		// mpProgress width 0px defaults to original size, so hide that too
		mpProgress->disabled = true;
		mpProgressEndCap->disabled = true;
		mpProgressBot->disabled = true;
	}
	else
	{
		mpProgress->disabled = false;
		mpProgressEndCap->disabled = false;
		mpProgressBot->disabled = false;
	}
}

void Player::Hotbar_t::updateHotbar()
{
	if ( !hotbarFrame )
	{
		return;
	}

	const int hotbarStartY1 = hotbarFrame->getSize().h - 106; // higher row (center group)
	const int hotbarStartY2 = hotbarFrame->getSize().h - 96; // lower row (left/right)
	const int hotbarCentreX = hotbarFrame->getSize().w / 2;
	const int hotbarCentreXLeft = hotbarCentreX - 148;
	const int hotbarCentreXRight = hotbarCentreX + 148;

	if ( !player.shootmode )
	{
		if ( Input::inputs[player.playernum].binaryToggle("HotbarFacebarCancel") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("HotbarFacebarCancel");
		}
		if ( Input::inputs[player.playernum].binaryToggle("HotbarFacebarLeft") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("HotbarFacebarLeft");
			Input::inputs[player.playernum].consumeBinaryReleaseToggle("HotbarFacebarLeft");
		}
		if ( Input::inputs[player.playernum].binaryToggle("HotbarFacebarUp") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("HotbarFacebarUp");
			Input::inputs[player.playernum].consumeBinaryReleaseToggle("HotbarFacebarUp");
		}
		if ( Input::inputs[player.playernum].binaryToggle("HotbarFacebarRight") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("HotbarFacebarRight");
			Input::inputs[player.playernum].consumeBinaryReleaseToggle("HotbarFacebarRight");
		}
		faceMenuButtonHeld = FaceMenuGroup::GROUP_NONE;
	}

	bool faceMenuSnapCursorInstantly = false;
	if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_NONE )
	{
		if ( selectedSlotAnimateCurrentValue == 0.0 )
		{
			faceMenuSnapCursorInstantly = true;
		}
		real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		real_t setpointDiff = std::max(0.1, 1.0 - selectedSlotAnimateCurrentValue);
		selectedSlotAnimateCurrentValue += fpsScale * (setpointDiff / 10.0);
		selectedSlotAnimateCurrentValue = std::min(1.0, selectedSlotAnimateCurrentValue);
	}
	else
	{
		selectedSlotAnimateCurrentValue = 0.0;
	}

	auto highlightSlot = hotbarFrame->findFrame("hotbar highlight");
	auto highlightSlotImg = highlightSlot->findImage("highlight img");
	highlightSlotImg->disabled = true;

	auto shootmodeSelectedSlotCursor = hotbarFrame->findFrame("shootmode selected item cursor");
	if ( shootmodeSelectedSlotCursor )
	{
		shootmodeSelectedSlotCursor->setDisabled(true);
	}

	if ( player.shootmode || !inputs.getUIInteraction(player.playernum)->selectedItem )
	{
		if ( auto oldSelectedItemFrame = hotbarFrame->findFrame("hotbar old selected item") )
		{
			oldSelectedItemFrame->setDisabled(true);
		}
	}

	// position the slots
	for ( int num = 0; num < NUM_HOTBAR_SLOTS; ++num )
	{
		if ( hotbar[num].item != 0 )
		{
			hotbar[num].lastItemUid = hotbar[num].item;
			if ( Item* item = uidToItem(hotbar[num].item) )
			{
				hotbar[num].lastItemType = item->type;
				hotbar[num].lastItemCategory = itemCategory(item);
			}
		}

		char slotname[32];
		snprintf(slotname, sizeof(slotname), "hotbar slot %d", num);
		auto slot = hotbarFrame->findFrame(slotname);
		assert(slot);

		if ( auto img = slot->findImage("slot img") ) // apply any opacity from config
		{
			Uint8 r, g, b, a;
			SDL_GetRGBA(img->color, mainsurface->format, &r, &g, &b, &a);
			a = hotbarSlotOpacity;
			img->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}
		if ( highlightSlotImg )
		{
			Uint8 r, g, b, a;
			SDL_GetRGBA(highlightSlotImg->color, mainsurface->format, &r, &g, &b, &a);
			a = hotbarSelectedSlotOpacity;
			highlightSlotImg->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		}

		char glyphname[32];
		snprintf(glyphname, sizeof(glyphname), "hotbar glyph %d", num);
		auto glyph = hotbarFrame->findImage(glyphname);
		assert(glyph);
		glyph->disabled = true;

		if ( useHotbarFaceMenu && num == 9 )
		{
			slot->setDisabled(true);
		}
		else
		{
			slot->setDisabled(false);
		}

		SDL_Rect pos = slot->getSize();
		pos.x = hotbarCentreX;
		pos.y = hotbarStartY2;

		int slotYMovement = pos.h / 4;

		auto slotItem = slot->findFrame("hotbar slot item");
		slotItem->setDisabled(true);

		if ( useHotbarFaceMenu )
		{
			GameController* controller = inputs.getController(player.playernum);
			if ( controller )
			{
				glyph->disabled = slot->isDisabled();
			}

			slot->findField("slot num text")->setDisabled(true); // disable the hotkey prompts per slot
			switch ( num )
			{
				// left group
				case 0:
					pos.x = hotbarCentreXLeft - pos.w / 2 - pos.w + 2;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_LEFT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarModifierLeft");
					break;
				case 1:
					pos.x = hotbarCentreXLeft - pos.w / 2;
					pos.y -= slotYMovement;
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarLeft");
					break;
				case 2:
					pos.x = hotbarCentreXLeft + (pos.w / 2 - 2);
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_LEFT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarModifierRight");
					break;
				// middle group
				case 3:
					pos.y = hotbarStartY1;
					pos.x = hotbarCentreX - pos.w / 2 - pos.w + 2;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarModifierLeft");
					break;
				case 4:
					pos.y = hotbarStartY1;
					pos.y -= slotYMovement;
					pos.x = hotbarCentreX - pos.w / 2;
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarUp");
					break;
				case 5:
					pos.y = hotbarStartY1;
					pos.x = hotbarCentreX + (pos.w / 2 - 2);
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarModifierRight");
					break;
				// right group
				case 6:
					pos.x = hotbarCentreXRight - pos.w / 2 - pos.w + 2;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_RIGHT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarModifierLeft");
					break;
				case 7:
					pos.x = hotbarCentreXRight - pos.w / 2;
					pos.y -= slotYMovement;
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarRight");
					break;
				case 8:
					pos.x = hotbarCentreXRight + (pos.w / 2 - 2);
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_RIGHT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForInput("HotbarFacebarModifierRight");
					break;
				default:
					break;
			}
		}
		else
		{
			slot->findField("slot num text")->setDisabled(false); // enable the hotkey prompts per slot
			const unsigned int midpoint = NUM_HOTBAR_SLOTS / 2;
			if ( num < midpoint )
			{
				pos.x -= (pos.w) * (midpoint - num);
			}
			else
			{
				pos.x += (pos.w) * (num - midpoint);
			}
		}

		slot->setSize(pos);

		auto glyphImage = Image::get(glyph->path.c_str());
		if ( glyphImage )
		{
			glyph->pos.w = glyphImage->getWidth();
			glyph->pos.h = glyphImage->getHeight();
			glyph->pos.x = pos.x + pos.w / 2 - glyph->pos.w / 2;
			glyph->pos.y = pos.y - glyph->pos.h;
		}

		if ( current_hotbar == num )
		{
			bool showHighlightedSlot = true;
			if ( players[player.playernum]->GUI.activeModule != Player::GUI_t::MODULE_HOTBAR
				&& player.inventoryUI.frame && !player.inventoryUI.frame->isDisabled() )
			{
				// if inventory visible, don't show selection if navigating within inventory
				showHighlightedSlot = false;
			}
			else if ( player.hotbar.useHotbarFaceMenu && inputs.getVirtualMouse(player.playernum)->lastMovementFromController
				&& player.hotbar.faceMenuButtonHeld == Player::Hotbar_t::FaceMenuGroup::GROUP_NONE
				&& player.inventoryUI.frame && player.inventoryUI.frame->isDisabled() )
			{
				// if inventory invisible, don't show selection if face button not held
				showHighlightedSlot = false;
			}

			auto highlightSlotItem = highlightSlot->findFrame("hotbar slot item");
			highlightSlotItem->setDisabled(true);

			if ( showHighlightedSlot )
			{
				auto slotNumText = slot->findField("slot num text");
				auto highlightNumText = highlightSlot->findField("slot num text");

				highlightNumText->setText(slotNumText->getText());
				highlightNumText->setDisabled(slotNumText->isDisabled());

				highlightSlot->setSize(pos); // this follows the slots around
				highlightSlotImg->disabled = false;
				updateSlotFrameFromItem(highlightSlotItem, uidToItem(hotbar[num].item));

				if ( player.inventoryUI.frame )
				{
					bool showCursor = true;
					if ( !player.shootmode )
					{
						if ( inputs.getUIInteraction(player.playernum)->selectedItem 
							&& !highlightSlot->capturesMouseInRealtimeCoords() )
						{
							showCursor = false;
						}
						else if ( !inputs.getUIInteraction(player.playernum)->selectedItem
							&& !inputs.getVirtualMouse(player.playernum)->lastMovementFromController
							&& !highlightSlot->capturesMouse() )
						{
							showCursor = false;
						}
					}
					else if ( player.shootmode )
					{
						showCursor = true;
					}

					if ( showCursor )
					{
						if ( !player.shootmode )
						{
							if ( players[player.playernum]->inventoryUI.selectedItemCursorFrame )
							{
								players[player.playernum]->inventoryUI.selectedItemCursorFrame->setDisabled(false);
								player.inventoryUI.updateSelectedSlotAnimation(pos.x - 1, pos.y - 1, getSlotSize(), getSlotSize(), 
									inputs.getVirtualMouse(player.playernum)->draw_cursor);
							}
						}
						else if ( player.shootmode )
						{
							if ( shootmodeSelectedSlotCursor )
							{
								shootmodeSelectedSlotCursor->setDisabled(false);
								bool snapCursor = !inputs.getVirtualMouse(player.playernum)->lastMovementFromController;
								if ( useHotbarFaceMenu && faceMenuSnapCursorInstantly )
								{
									snapCursor = true;
								}
								updateSelectedSlotAnimation(pos.x - 1, pos.y - 1, getSlotSize(), getSlotSize(), snapCursor);
							}
						}
					}
					else
					{
						highlightSlotImg->disabled = true;
					}
				}
			}
			else
			{
				updateSlotFrameFromItem(slotItem, uidToItem(hotbar[num].item));
			}
		}
		else
		{
			updateSlotFrameFromItem(slotItem, uidToItem(hotbar[num].item));
		}
	}
}

bool Player::Hotbar_t::warpMouseToHotbar(const int hotbarSlot, Uint32 flags)
{
	if ( !hotbarFrame || hotbarSlot < 0 || hotbarSlot >= NUM_HOTBAR_SLOTS )
	{
		return false;
	}
	if ( auto slotFrame = getHotbarSlotFrame(hotbarSlot) )
	{
		slotFrame->warpMouseToFrame(player.playernum, flags);
		return true;
	}
	return false;
}

Frame* Player::Hotbar_t::getHotbarSlotFrame(const int hotbarSlot)
{
	if ( !hotbarFrame || hotbarSlot < 0 || hotbarSlot >= NUM_HOTBAR_SLOTS )
	{
		return nullptr;
	}

	char slotname[32];
	snprintf(slotname, sizeof(slotname), "hotbar slot %d", hotbarSlot);
	return hotbarFrame->findFrame(slotname);
}

static Uint32 gui_ticks = 0u;
void doFrames() {
	if ( gui )
	{
		while ( gui_ticks < ticks )
		{
			++gui_ticks;
		}
		(void)gui->process();
		gui->draw();
	}
}

real_t Player::SkillSheet_t::windowCompactHeightScaleX = 0.0;
real_t Player::SkillSheet_t::windowCompactHeightScaleY = 0.0;
real_t Player::SkillSheet_t::windowHeightScaleX = 0.0;
real_t Player::SkillSheet_t::windowHeightScaleY = 0.0;
bool Player::SkillSheet_t::generateFollowerTableForSkillsheet = false;

void Player::SkillSheet_t::createSkillSheet()
{
	if ( skillFrame )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player skills %d", player.playernum);
	Frame* frame = gui->addFrame(name);
	skillFrame = frame;
	frame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });
	frame->setHollow(false);
	frame->setBorder(0);
	frame->setOwner(player.playernum);
	frame->setInheritParentFrameOpacity(false);

	auto fade = skillFrame->addImage(
		SDL_Rect{ 0, 0, skillFrame->getSize().w, skillFrame->getSize().h },
		0, "images/system/white.png", "fade img");
	Uint8 r, g, b, a;
	SDL_GetRGBA(fade->color, mainsurface->format, &r, &g, &b, &a);
	a = 0;
	fade->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);

	Frame* skillBackground = frame->addFrame("skills frame");
	const int width = 684;
	const int height = 404;
	skillBackground->setSize(SDL_Rect{ frame->getSize().x, frame->getSize().y, width, height });
	/*auto bgImg = skillBackground->addImage(SDL_Rect{ 0, 0, width, height }, 0xFFFFFFFF,
		"images/ui/SkillSheet/UI_Skills_Window_02.png", "skills img");*/

	Frame* allSkillEntriesLeft = skillBackground->addFrame("skill entries frame left");
	Frame* allSkillEntriesRight = skillBackground->addFrame("skill entries frame right");
	//allSkillEntries->setSize(SDL_Rect{ 0, 0, skillBackground->getSize().w, skillBackground->getSize().h });

	SDL_Rect allSkillEntriesPosLeft{ 0, 0, 182, skillBackground->getSize().h };
	SDL_Rect allSkillEntriesPosRight{ skillBackground->getSize().w - 182, 0, 182, skillBackground->getSize().h };
	allSkillEntriesLeft->setSize(allSkillEntriesPosLeft);
	allSkillEntriesRight->setSize(allSkillEntriesPosRight);

	allSkillEntriesLeft->addImage(SDL_Rect{ 0, 12, 182, 376 },
		0xFFFFFFFF, "images/ui/SkillSheet/UI_Skills_Window_Left_03.png", "bg wing left");
	allSkillEntriesRight->addImage(SDL_Rect{ 0, 12, 182, 376 },
		0xFFFFFFFF, "images/ui/SkillSheet/UI_Skills_Window_Right_03.png", "bg wing right");

	auto skillBackgroundImagesFrame = skillBackground->addFrame("skills bg images");
	skillBackgroundImagesFrame->setSize(SDL_Rect{ 0, 0, skillBackground->getSize().w, skillBackground->getSize().h });
	{
		Uint32 color = makeColor(255, 255, 255, 255);
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_TL_04.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_TR_04.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_T_04.png", skillsheetEffectBackgroundImages[TOP].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_L_03.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_R_03.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			makeColor(0, 0, 0, 255), "images/system/white.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_BL_03.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_BR_03.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "images/ui/SkillSheet/UI_Skills_Window_B_03.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
		imageSetWidthHeight9x9(skillBackgroundImagesFrame, skillsheetEffectBackgroundImages);

		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 78, 18 },
			color, "images/ui/SkillSheet/UI_Skills_Window_Flourish_T.png", "flourish top");
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 34, 14 },
			color, "images/ui/SkillSheet/UI_Skills_Window_Flourish_B.png", "flourish bottom");
	}

	const int skillEntryStartY = 38;
	SDL_Rect skillEntryPos{ 0, skillEntryStartY, 182, 40 };
	SDL_Rect skillSelectorPos{ 0, 2, 146, 32 };
	const char* boldFont = "fonts/pixel_maz_multiline.ttf#16#2";
	const char* numberFont = "fonts/pixelmix.ttf#16";
	//const char* titleFont = "fonts/pixel_maz_multiline.ttf#24#2";
	const char* titleFont = "fonts/pixelmix.ttf#24#2";
	const char* descFont = "fonts/pixel_maz_multiline.ttf#16#2";
	for ( int i = 0; i < 8; ++i )
	{
		char skillname[32];
		snprintf(skillname, sizeof(skillname), "skill %d", i);
		Frame* entry = allSkillEntriesRight->addFrame(skillname);
		entry->setSize(skillEntryPos);
		entry->addImage(skillSelectorPos, 0xFFFFFFFF, "images/ui/SkillSheet/UI_Skills_SkillSelector_00.png", "selector img");
		SDL_Rect imgBgPos{ skillEntryPos.w - 36, 0, 36, 36 };
		entry->addImage(imgBgPos, 0xFFFFFFFF, "images/ui/SkillSheet/UI_Skills_Icons_BG00_00.png", "skill icon bg");
		SDL_Rect imgFgPos{ imgBgPos.x + 6, imgBgPos.y + 6, 24, 24 };
		entry->addImage(imgFgPos, 0xFFFFFFFF, "", "skill icon fg");
		SDL_Rect statPos{ 10, 6, 24, 24 };
		auto statIcon = entry->addImage(statPos, 0xFFFFFFFF, "", "stat icon");
		if ( i < skillSheetData.skillEntries.size() )
		{
			statIcon->path = skillSheetData.skillEntries[i].statIconPath;
		}
		skillEntryPos.y += skillEntryPos.h;

		SDL_Rect profNamePos{ statPos.x + statPos.w - 32, 4, 98, 28 };
		Field* profName = entry->addField("skill name", 64);
		profName->setSize(profNamePos);
		profName->setHJustify(Field::justify_t::RIGHT);
		profName->setVJustify(Field::justify_t::CENTER);
		profName->setFont(boldFont);
		profName->setColor(skillSheetData.defaultTextColor);
		if ( i < skillSheetData.skillEntries.size() )
		{
			profName->setText(skillSheetData.skillEntries[i].name.c_str());
		}
		else
		{
			profName->setText("Default");
		}

		SDL_Rect profLevelPos{ profNamePos.x + profNamePos.w + 8 - 2, profNamePos.y, 34, 28 };
		Field* profLevel = entry->addField("skill level", 16);
		profLevel->setSize(profLevelPos);
		profLevel->setHJustify(Field::justify_t::RIGHT);
		profLevel->setVJustify(Field::justify_t::CENTER);
		profLevel->setFont(numberFont);
		profLevel->setColor(skillSheetData.defaultTextColor);
		profLevel->setText("0");
	}

	skillEntryPos.x = 0;
	skillEntryPos.y = skillEntryStartY;
	skillSelectorPos.x = 36;

	for ( int i = 8; i < 16; ++i )
	{
		char skillname[32];
		snprintf(skillname, sizeof(skillname), "skill %d", i);
		Frame* entry = allSkillEntriesLeft->addFrame(skillname);
		entry->setSize(skillEntryPos);
		entry->addImage(skillSelectorPos, 0xFFFFFFFF, "images/ui/SkillSheet/UI_Skills_SkillSelectorR_00.png", "selector img");
		SDL_Rect imgBgPos{ 0, 0, 36, 36 };
		entry->addImage(imgBgPos, 0xFFFFFFFF, "images/ui/SkillSheet/UI_Skills_Icons_BG00_00.png", "skill icon bg");
		SDL_Rect imgFgPos{ imgBgPos.x + 6, imgBgPos.y + 6, 24, 24 };
		entry->addImage(imgFgPos, 0xFFFFFFFF, "images/ui/SkillSheet/icons/Alchemy01.png", "skill icon fg");
		SDL_Rect statPos{ skillEntryPos.w - 10 - 24, 6, 24, 24 };
		auto statIcon = entry->addImage(statPos, 0xFFFFFFFF, "", "stat icon");
		if ( i < skillSheetData.skillEntries.size() )
		{
			statIcon->path = skillSheetData.skillEntries[i].statIconPath;
		}
		skillEntryPos.y += skillEntryPos.h;

		SDL_Rect profNamePos{ statPos.x - 98 + 32, 4, 98, 28 };
		Field* profName = entry->addField("skill name", 64);
		profName->setSize(profNamePos);
		profName->setHJustify(Field::justify_t::LEFT);
		profName->setVJustify(Field::justify_t::CENTER);
		profName->setFont(boldFont);
		profName->setColor(skillSheetData.defaultTextColor);
		if ( i < skillSheetData.skillEntries.size() )
		{
			profName->setText(skillSheetData.skillEntries[i].name.c_str());
		}
		else
		{
			profName->setText("Default");
		}

		SDL_Rect profLevelPos{ profNamePos.x - 12 - 32, profNamePos.y, 34, 28 };
		Field* profLevel = entry->addField("skill level", 16);
		profLevel->setSize(profLevelPos);
		profLevel->setHJustify(Field::justify_t::RIGHT);
		profLevel->setVJustify(Field::justify_t::CENTER);
		profLevel->setFont(numberFont);
		profLevel->setColor(skillSheetData.defaultTextColor);
		profLevel->setText("0");
	}

	SDL_Rect skillTitlePos{ skillBackground->getSize().w / 2 - 320 / 2, 14, 320, 40 };
	auto skillTitleTxt = skillBackground->addField("skill title txt", 64);
	skillTitleTxt->setHJustify(Field::justify_t::CENTER);
	skillTitleTxt->setVJustify(Field::justify_t::CENTER);
	skillTitleTxt->setText("");
	skillTitleTxt->setSize(skillTitlePos);
	skillTitleTxt->setFont(titleFont);
	skillTitleTxt->setOntop(true);
	skillTitleTxt->setColor(makeColor(201, 162, 100, 255));

	SDL_Rect descPos{ 0, 54, 320, 324 };
	descPos.x = skillBackground->getSize().w / 2 - descPos.w / 2;
	auto skillDescriptionFrame = skillBackground->addFrame("skill desc frame");
	skillDescriptionFrame->setSize(descPos);

	/*auto debugRect = skillDescriptionFrame->addImage(SDL_Rect{ 0, 0, descPos.w, descPos.h }, 0xFFFFFFFF,
		"images/system/white.png", "")*/;

	auto slider = skillDescriptionFrame->addSlider("skill slider");
	slider->setBorder(24);
	slider->setMinValue(0);
	slider->setMaxValue(100);
	slider->setValue(0);
	SDL_Rect sliderPos{ descPos.w - 34, 4, 30, descPos.h - 8 };
	slider->setRailSize(SDL_Rect{ sliderPos });
	slider->setHandleSize(SDL_Rect{ 0, 0, 34, 34 });
	slider->setOrientation(Slider::SLIDER_VERTICAL);
	//slider->setCallback(callback);
	slider->setColor(makeColor(255, 255, 255, 255));
	slider->setHighlightColor(makeColor(255, 255, 255, 255));
	slider->setHandleImage("images/ui/Main Menus/Settings/Settings_Slider_Boulder00.png");
	slider->setRailImage("images/ui/Main Menus/Settings/Settings_Slider_Backing00.png");
	slider->setHideGlyphs(true);

	Font* actualFont = Font::get(descFont);
	int fontHeight;
	actualFont->sizeText("_", nullptr, &fontHeight);

	auto scrollAreaOuterFrame = skillDescriptionFrame->addFrame("scroll area outer frame");
	scrollAreaOuterFrame->setSize(SDL_Rect{ 16, 4, sliderPos.x - 4 - 16, descPos.h - 8 });
	auto scrollAreaFrame = scrollAreaOuterFrame->addFrame("skill scroll area");
	scrollAreaFrame->setSize(SDL_Rect{ 0, 0, scrollAreaOuterFrame->getSize().w, 1000 });

	SDL_Rect txtPos{ 0, 0, scrollAreaFrame->getSize().w, scrollAreaFrame->getSize().h };
	{
		auto skillLvlHeaderTxt = scrollAreaFrame->addField("skill lvl header txt", 128);
		skillLvlHeaderTxt->setFont(descFont);
		skillLvlHeaderTxt->setSize(txtPos);
		skillLvlHeaderTxt->setText("Skill Level:");

		auto skillLvlHeaderVal = scrollAreaFrame->addField("skill lvl header val", 128);
		skillLvlHeaderVal->setFont(descFont);
		skillLvlHeaderVal->setSize(txtPos);
		skillLvlHeaderVal->setText("");
		skillLvlHeaderVal->setHJustify(Field::justify_t::RIGHT);

		txtPos.y += fontHeight;
	}
	{
		const int iconSize = 24;
		auto statIcon = scrollAreaFrame->addImage(
			SDL_Rect{ txtPos.x + txtPos.w - iconSize, txtPos.y, iconSize, iconSize },
			0xFFFFFFFF, "images/ui/CharSheet/HUD_CharSheet_DEX_00.png",	"stat icon");

		txtPos.y += std::max(0, iconSize - fontHeight);
		auto statHeaderTxt = scrollAreaFrame->addField("stat header txt", 128);
		statHeaderTxt->setFont(descFont);
		statHeaderTxt->setSize(txtPos);
		statHeaderTxt->setText("Associated Stat:");

		auto statTypeTxt = scrollAreaFrame->addField("stat type txt", 128);
		statTypeTxt->setFont(descFont);
		txtPos.w -= iconSize + 4;
		statTypeTxt->setSize(txtPos);
		txtPos.w += iconSize + 4;
		statTypeTxt->setText("");
		statTypeTxt->setHJustify(Field::justify_t::RIGHT);
		txtPos.y += fontHeight;
		txtPos.y += fontHeight / 2;
	}
	{
		auto skillDescriptionTxt = scrollAreaFrame->addField("skill desc txt", 1024);
		skillDescriptionTxt->setFont(descFont);
		skillDescriptionTxt->setSize(txtPos);
		skillDescriptionTxt->setText("");
		//skillDescriptionTxt->setColor(makeColor(201, 162, 100, 255));
		skillDescriptionTxt->setHJustify(Field::justify_t::CENTER);
		skillDescriptionTxt->setOntop(true);

		auto skillDescriptionBgFrame = scrollAreaFrame->addFrame("skill desc bg frame");
		{
			//Uint32 color = makeColor(22, 24, 29, 255);
			Uint32 color = makeColor(255, 255, 255, 128);
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_TL_00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_TR_00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_T_00.png", skillsheetEffectBackgroundImages[TOP].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_ML_00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_MR_00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_M_00.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_BL_00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_BR_00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "images/ui/SkillSheet/UI_Skills_LegendBox_B_00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
			imageSetWidthHeight9x9(skillDescriptionBgFrame, skillsheetEffectBackgroundImages);
		}


		int txtHeight = skillDescriptionTxt->getNumTextLines() * actualFont->height(true);

		txtPos.y += txtHeight;
		txtPos.y += fontHeight / 2;

	}
	{
		char effectFrameName[64] = "";
		char effectFieldName[64] = "";
		char effectFieldVal[64] = "";
		char effectBgName[64];
		int effectXOffset = 72; // tmp paramters - configured in skillsheet json
		int effectBackgroundXOffset = 8;
		int effectBackgroundWidth = 80;
		for ( int i = 0; i < 10; ++i )
		{
			snprintf(effectFrameName, sizeof(effectFrameName), "effect %d frame", i);
			auto effectFrame = scrollAreaFrame->addFrame(effectFrameName);
			effectFrame->setSize(SDL_Rect{ txtPos.x, txtPos.y - 2, txtPos.w, fontHeight + 8 });
			effectFrame->addImage(
				SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h - 4 }, 
				makeColor(101, 87, 67, 255), "images/system/white.png", "effect frame bg highlight");
			effectFrame->addImage(
				SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h - 4 },
				makeColor(101, 33, 33, 28), "images/system/white.png", "effect frame bg tmp");
			int valueX =  effectFrame->getSize().w - effectXOffset;

			auto valBgImgFrame = effectFrame->addFrame("effect val bg frame");
			valBgImgFrame->setSize(SDL_Rect{ valueX - effectBackgroundXOffset, 0, effectBackgroundWidth, effectFrame->getSize().h - 4});
			{
				Uint32 color = makeColor(51, 33, 26, 255);
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_TL00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_TR00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_T00.png", skillsheetEffectBackgroundImages[TOP].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_L00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_R00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_M00.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_BL00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_BR00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "images/ui/SkillSheet/UI_Skills_EffectBG_B00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());

				/*valBgImgFrame->addImage(
					SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h - 4 },
					makeColor(255, 255, 255, 128), "images/system/white.png", "tmp tmp");*/
				imageSetWidthHeight9x9(valBgImgFrame, skillsheetEffectBackgroundImages);
			}

			auto effectTxtFrame = effectFrame->addFrame("effect txt frame");
			effectTxtFrame->setSize(SDL_Rect{ 0, 0, effectFrame->getSize().w - effectXOffset - effectBackgroundXOffset, effectFrame->getSize().h - 4 });
			auto effectTxt = effectTxtFrame->addField("effect txt", 1024);
			effectTxt->setFont(descFont);
			effectTxt->setSize(SDL_Rect{0, 0, 1000, effectTxtFrame->getSize().h}); // large 1000px to handle large text length marquee
			effectTxt->setVJustify(Field::justify_t::CENTER);
			effectTxt->setText("");
			effectTxt->setColor(makeColor(201, 162, 100, 255));

			auto effectValFrame = effectFrame->addFrame("effect val frame");
			effectValFrame->setSize(SDL_Rect{ valueX, 0, effectXOffset, effectFrame->getSize().h - 4 });
			auto effectVal = effectValFrame->addField("effect val", 1024);
			effectVal->setFont(descFont);
			effectVal->setSize(SDL_Rect{ 0, 0, 1000, effectValFrame->getSize().h }); // large 1000px to handle large text length marquee
			effectVal->setVJustify(Field::justify_t::CENTER);
			effectVal->setText("");
			effectVal->setColor(makeColor(201, 162, 100, 255));

			//effectFrame->setDisabled(true);

			txtPos.y += effectFrame->getSize().h;
		}
	}
	{
		auto legendDivImg = scrollAreaFrame->addImage(SDL_Rect{ 0, txtPos.y, 212, 28 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_Separator_00.png", "legend div");
		legendDivImg->pos.x = scrollAreaFrame->getSize().w / 2 - legendDivImg->pos.w / 2;
		auto legendDivTxt = scrollAreaFrame->addField("legend div text", 128);
		legendDivTxt->setText(language[4056]);
		SDL_Rect legendDivTxtPos = legendDivImg->pos;
		legendDivTxt->setSize(legendDivTxtPos);
		legendDivTxt->setFont(descFont);
		legendDivTxt->setHJustify(Field::justify_t::CENTER);

		auto legendFrame = scrollAreaFrame->addFrame("legend frame");
		SDL_Rect legendPos{ 0, txtPos.y, txtPos.w, 100 };
		legendFrame->setSize(legendPos);
		auto tl = legendFrame->addImage(SDL_Rect{ 0, 0, 18, 18 }, 0xFFFFFFFF, 
			"images/ui/SkillSheet/UI_Skills_LegendBox_TL_00.png", "top left img");
		auto tm = legendFrame->addImage(SDL_Rect{ 18, 0, legendPos.w - 18 * 2, 18 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_T_00.png", "top img");
		auto tr = legendFrame->addImage(SDL_Rect{ legendPos.w - 18, 0, 18, 18 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_TR_00.png", "top right img");

		auto ml = legendFrame->addImage(SDL_Rect{ 0, 18, 18, legendPos.h - 18 * 2 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_ML_00.png", "middle left img");
		auto mm = legendFrame->addImage(SDL_Rect{ 18, 18, legendPos.w - 18 * 2, ml->pos.h }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_M_00.png", "middle img");
		auto mr = legendFrame->addImage(SDL_Rect{ legendPos.w - 18, 18, 18, ml->pos.h }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_MR_00.png", "middle right img");

		auto bl = legendFrame->addImage(SDL_Rect{ 0, ml->pos.y + ml->pos.h, 18, 18 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_BL_00.png", "bottom left img");
		auto bm = legendFrame->addImage(SDL_Rect{ 18, bl->pos.y, legendPos.w - 18 * 2, 18 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_B_00.png", "bottom img");
		auto br = legendFrame->addImage(SDL_Rect{ legendPos.w - 18, bl->pos.y, 18, 18 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendBox_BR_00.png", "bottom right img");

		auto backerImg = scrollAreaFrame->addImage(SDL_Rect{ 0, 0, 154, 12 }, 0xFFFFFFFF,
			"images/ui/SkillSheet/UI_Skills_LegendTextBacker_00.png", "legend txt backer img");
		backerImg->disabled = true;

		auto legendText = legendFrame->addField("legend text", 256);
		legendText->setText("");
		legendText->setSize(mm->pos);
		legendText->setFont(descFont);
		legendText->setHJustify(Field::justify_t::CENTER);
	}
	
	std::string promptFont = "fonts/pixel_maz.ttf#32#2";
	const int promptWidth = 60;
	const int promptHeight = 27;
	auto promptBack = frame->addField("prompt back txt", 16);
	promptBack->setSize(SDL_Rect{ frame->getSize().w - promptWidth - 16, // lower right corner
		0, promptWidth, promptHeight });
	promptBack->setFont(promptFont.c_str());
	promptBack->setHJustify(Field::justify_t::RIGHT);
	promptBack->setVJustify(Field::justify_t::CENTER);
	promptBack->setText(language[4053]);
	//promptBack->setOntop(true);
	promptBack->setColor(makeColor(201, 162, 100, 255));

	auto promptBackImg = frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
		"", "prompt back img");
	promptBackImg->disabled = true;
	//promptBackImg->ontop = true;

	auto promptScroll = frame->addField("prompt scroll txt", 16);
	promptScroll->setSize(SDL_Rect{ frame->getSize().w - promptWidth - 16, // lower right corner
		0, promptWidth, promptHeight });
	promptScroll->setFont(promptFont.c_str());
	promptScroll->setHJustify(Field::justify_t::LEFT);
	promptScroll->setVJustify(Field::justify_t::CENTER);
	promptScroll->setText(language[4062]);
	//promptScroll->setOntop(true);
	promptScroll->setColor(makeColor(201, 162, 100, 255));

	auto promptScrollImg = frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
		"", "prompt scroll img");
	promptScrollImg->disabled = true;
	promptScrollImg->ontop = true;
}

void Player::SkillSheet_t::resetSkillDisplay()
{
	bSkillSheetEntryLoaded = false;
	scrollInertia = 0.0;

	for ( auto& skillEntry : skillSheetData.skillEntries )
	{
		for ( auto& skillEffect : skillEntry.effects )
		{
			skillEffect.marqueeTicks = 0;
			skillEffect.marquee = 0.0;
			skillEffect.effectUpdatedAtSkillLevel = -1;
		}
	}
}

void Player::SkillSheet_t::closeSkillSheet()
{
	bSkillSheetOpen = false;
	if ( skillFrame )
	{
		skillFrame->setDisabled(true);
	}
	resetSkillDisplay();

	if ( player.gui_mode == GUI_MODE_NONE )
	{
		player.shootmode = true;
	}
}

void Player::SkillSheet_t::openSkillSheet()
{
	if ( player.shootmode )
	{
		players[player.playernum]->openStatusScreen(GUI_MODE_NONE,
			INVENTORY_MODE_ITEM, player.GUI.MODULE_SKILLS_LIST);
	}
	else
	{
		players[player.playernum]->openStatusScreen(GUI_MODE_INVENTORY,
			INVENTORY_MODE_ITEM, player.GUI.MODULE_SKILLS_LIST); // Reset the GUI to the inventory.
	}
	bSkillSheetOpen = true;
	openTick = ticks;
	scrollPercent = 0.0;
	if ( skillFrame )
	{
		auto innerFrame = skillFrame->findFrame("skills frame");
		auto skillDescriptionFrame = innerFrame->findFrame("skill desc frame");
		auto slider = skillDescriptionFrame->findSlider("skill slider");
		slider->setValue(0.0);
	}
	resetSkillDisplay();
	if ( selectedSkill < 0 )
	{
		selectSkill(0);
	}
	if ( !::inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		highlightedSkill = selectedSkill;
	}
}

std::string formatSkillSheetEffects(int playernum, int proficiency, std::string& tag, std::string& rawValue)
{
	char buf[1024] = "";
	if ( !players[playernum] ) { return ""; }
	Entity* player = players[playernum]->entity;
	real_t val = 0.0;
	real_t val2 = 0.0;
	if ( proficiency == PRO_STEALTH )
	{
		if ( tag == "STEALTH_VIS_REDUCTION" )
		{
			val = stats[playernum]->PROFICIENCIES[proficiency] * 2 * 100 / 512.f; // % visibility reduction
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "STEALTH_SNEAK_VIS" )
		{
			val = (2 + (stats[playernum]->PROFICIENCIES[proficiency] / 40)); // night vision when sneaking
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "STEALTH_BACKSTAB" )
		{
			val = (stats[playernum]->PROFICIENCIES[proficiency] / 20 + 2) * 2; // backstab dmg
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val *= 2;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "STEALTH_CURRENT_VIS" )
		{
			if ( player )
			{
				val = player->entityLightAfterReductions(*stats[playernum], nullptr);
				val = std::max(1, (static_cast<int>(val / 32.0))); // general visibility
				snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
			}
			else
			{
				return "-";
			}
		}
		return buf;
	}
	else if ( proficiency == PRO_RANGED )
	{
		if ( tag == "RANGED_DMG_RANGE" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 3.f; // lowest damage roll
			}
			else
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 2.f; // lowest damage roll
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "RANGED_DMG_EFFECTIVENESS" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			else
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "RANGED_DEGRADE_CHANCE" )
		{
			val = 50 + static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20) * 10;
			if ( stats[playernum]->type == GOBLIN )
			{
				val += 20;
				if ( stats[playernum]->PROFICIENCIES[proficiency] < SKILL_LEVEL_LEGENDARY )
				{
					val = std::min(val, 90.0);
				}
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "RANGED_THROWN_DMG" )
		{
			int skillLVL = stats[playernum]->PROFICIENCIES[proficiency] / 20; // thrown dmg bonus
			// +0% baseline
			val = 100 * (thrownDamageSkillMultipliers[std::min(skillLVL, 5)] - thrownDamageSkillMultipliers[0]);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "RANGED_PIERCE_CHANCE" )
		{
			val = std::min(std::max(statGetPER(stats[playernum], player) / 2, 0), 50);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		return buf;
	}
	else if ( proficiency == PRO_SHIELD )
	{
		if ( tag == "BLOCK_AC_INCREASE" )
		{
			val = 5 + static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 5);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "BLOCK_DEGRADE_NORMAL_CHANCE" )
		{
			val = 25 + (stats[playernum]->type == GOBLIN ? 10 : 0); // degrade > 0 dmg taken
			val += (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 10));
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "BLOCK_DEGRADE_DEFENDING_CHANCE" )
		{
			val = 10 + (stats[playernum]->type == GOBLIN ? 10 : 0); // degrade on 0 dmg
			val += (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 10));
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val = 40 + (stats[playernum]->type == GOBLIN ? 10 : 0);
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		return buf;
	}
	else if ( proficiency == PRO_UNARMED )
	{
		if ( tag == "UNARMED_DMG_RANGE" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 3.f; // lowest damage roll
			}
			else
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 2.f; // lowest damage roll
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "UNARMED_DMG_EFFECTIVENESS" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			else
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "UNARMED_BONUS_DMG" )
		{
			val = static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "GLOVE_DEGRADE_CHANCE" )
		{
			val = 100 + (stats[playernum]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "GLOVE_DEGRADE0_CHANCE" )
		{
			val = 8 + (stats[playernum]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "UNARMED_KNOCKBACK_DIST" )
		{
			val = static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20) * 20;
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		return buf;
	}
	else if ( proficiency == PRO_SWORD )
	{
		if ( tag == "SWORD_DMG_RANGE" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 3.f; // lowest damage roll
			}
			else
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 2.f; // lowest damage roll
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "SWORD_DMG_EFFECTIVENESS" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			else
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "SWORD_DEGRADE_CHANCE" )
		{
			val = 50 + (stats[playernum]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
			val += (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20)) * 10;
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "SWORD_DEGRADE0_CHANCE" )
		{
			val = 4 + (stats[playernum]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
			val += static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20);
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		return buf;
	}
	else if ( proficiency == PRO_POLEARM )
	{
		if ( tag == "POLEARM_DMG_RANGE" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 3.f; // lowest damage roll
			}
			else
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 2.f; // lowest damage roll
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "POLEARM_DMG_EFFECTIVENESS" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			else
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "POLEARM_DEGRADE_CHANCE" )
		{
			val = 50 + (stats[playernum]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
			val += (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20)) * 10;
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "POLEARM_DEGRADE0_CHANCE" )
		{
			val = 4 + (stats[playernum]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
			val += static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20);
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		return buf;
	}
	else if ( proficiency == PRO_AXE )
	{
		if ( tag == "AXE_DMG_RANGE" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 3.f; // lowest damage roll
			}
			else
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 2.f; // lowest damage roll
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "AXE_DMG_EFFECTIVENESS" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			else
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "AXE_DEGRADE_CHANCE" )
		{
			val = 50 + (stats[playernum]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
			val += (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20)) * 10;
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "AXE_DEGRADE0_CHANCE" )
		{
			val = 4 + (stats[playernum]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
			val += static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20);
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		return buf;
	}
	else if ( proficiency == PRO_MACE )
	{
		if ( tag == "MACE_DMG_RANGE" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 3.f; // lowest damage roll
			}
			else
			{
				val = 100 - (100 - stats[playernum]->PROFICIENCIES[proficiency]) / 2.f; // lowest damage roll
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "MACE_DMG_EFFECTIVENESS" )
		{
			if ( proficiency == PRO_POLEARM )
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			else
			{
				val = -25 + (stats[playernum]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "MACE_DEGRADE_CHANCE" )
		{
			val = 50 + (stats[playernum]->type == GOBLIN ? 20 : 0); // chance to degrade on > 0 dmg
			val += (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20)) * 10;
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "MACE_DEGRADE0_CHANCE" )
		{
			val = 4 + (stats[playernum]->type == GOBLIN ? 4 : 0); // chance to degrade on 0 dmg
			val += static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20);
			if ( svFlags & SV_FLAG_HARDCORE )
			{
				val *= 2;
			}
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				val = 0.0;
			}
			if ( val > 0.0001 )
			{
				val = 100 / val;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		return buf;
	}
	else if ( proficiency == PRO_SWIMMING )
	{
		if ( tag == "SWIM_SPEED_TOTAL" )
		{
			val = (((stats[playernum]->PROFICIENCIES[proficiency] / 100.f) * 50.f) + 50); // water movement speed
			if ( stats[playernum]->type == SKELETON )
			{
				val *= .5;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "SWIM_SPEED_BASE" )
		{
			val = -50.0; // water movement speed
			if ( stats[playernum]->type == SKELETON )
			{
				val -= 25.0;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "SWIM_SPEED_BONUS" )
		{
			val = (((stats[playernum]->PROFICIENCIES[proficiency] / 100.f) * 50.f)); // water movement speed
			if ( stats[playernum]->type == SKELETON )
			{
				val *= .5;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		return buf;
	}
	else if ( proficiency == PRO_LEADERSHIP )
	{
		if ( tag == "LEADER_MAX_FOLLOWERS" )
		{
			val = std::min(8, std::max(4, 2 * (stats[playernum]->PROFICIENCIES[proficiency] / 20))); // max followers
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "LEADER_FOLLOWER_SPEED" )
		{
			val = 1 + (stats[playernum]->PROFICIENCIES[proficiency] / 20);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "LEADER_CHARM_MONSTER" )
		{
			val = 80 + ((statGetCHR(stats[playernum], player) + stats[playernum]->PROFICIENCIES[proficiency]) / 20) * 10;
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "LIST_LEADER_AVAILABLE_FOLLOWERS" )
		{
			std::string outputList = "";
			Monster playerRace = stats[playernum]->type;
			std::map<Monster, std::vector<Monster>>* allyTable = &Player::SkillSheet_t::skillSheetData.leadershipAllyTableBase;
			enum TableOrder : int 
			{
				LEADER_BASE_TABLE,
				LEADER_UNIQUE_TABLE,
				LEADER_CAPSTONE_TABLE,
				TABLE_MAX
			};
			for ( int i = LEADER_BASE_TABLE; i < TABLE_MAX; ++i )
			{
				if ( i == LEADER_CAPSTONE_TABLE )
				{
					if ( skillCapstoneUnlocked(playernum, proficiency) )
					{
						allyTable = &Player::SkillSheet_t::skillSheetData.leadershipAllyTableLegendary;
					}
					else
					{
						continue;
					}
				}
				if ( i == LEADER_CAPSTONE_TABLE || i == LEADER_BASE_TABLE )
				{
					if ( allyTable->find(playerRace) != allyTable->end() )
					{
						if ( !(*allyTable)[playerRace].empty() )
						{
							for ( auto& ally : (*allyTable)[playerRace] )
							{
								if ( ally < 0 || ally >= NUMMONSTERS ) { continue; }
								std::string monsterName = "";
								monsterName = getMonsterLocalizedName(ally);
								capitalizeString(monsterName);
								if ( outputList != "" )
								{
									outputList += '\n';
								}
								outputList += "\x1E " + monsterName;
							}
						}
					}
				}
				else if ( i == LEADER_UNIQUE_TABLE )
				{
					auto* allyTable = &Player::SkillSheet_t::skillSheetData.leadershipAllyTableSpecialRecruitment;
					if ( allyTable->find(playerRace) != allyTable->end() )
					{
						if ( !(*allyTable)[playerRace].empty() )
						{
							for ( auto& allyPair : (*allyTable)[playerRace] )
							{
								auto& ally = allyPair.first;
								if ( ally < 0 || ally >= NUMMONSTERS ) { continue; }
								std::string monsterName = "";
								monsterName = getMonsterLocalizedName(ally);
								capitalizeString(monsterName);
								if ( outputList != "" )
								{
									outputList += '\n';
								}
								outputList += "\x1E " + monsterName;
								outputList += "\n" + allyPair.second;
							}
						}
					}
				}
			}
			if ( outputList == "" ) { outputList = "-"; }
			snprintf(buf, sizeof(buf), rawValue.c_str(), outputList.c_str());
		}
		return buf;
	}
	else if ( proficiency == PRO_TRADING )
	{
		if ( tag == "TRADING_BUY_PRICE" )
		{
			val = 1 / ((50 + stats[playernum]->PROFICIENCIES[proficiency]) / 150.f); // buy value
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "TRADING_SELL_PRICE" )
		{
			val = (50 + stats[playernum]->PROFICIENCIES[proficiency]) / 150.f; // sell value
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		return buf;
	}
	else if ( proficiency == PRO_APPRAISAL )
	{
		if ( tag == "APPRAISE_GOLD_SPEED" )
		{
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				snprintf(buf, sizeof(buf), "%s", language[4064]); // "instant"
			}
			else
			{
				val = (60.f / (stats[playernum]->PROFICIENCIES[proficiency] + 1)) / (TICKS_PER_SECOND); // appraisal time per gold value
				snprintf(buf, sizeof(buf), rawValue.c_str(), val);
			}
		}
		else if ( tag == "APPRAISE_MAX_GOLD_VALUE" )
		{
			if ( skillCapstoneUnlocked(playernum, proficiency) )
			{
				snprintf(buf, sizeof(buf), "%s", language[4065]); // "any"
			}
			else
			{
				val = 10 * (stats[playernum]->PROFICIENCIES[proficiency] + (statGetPER(stats[playernum], player) * 5)); // max gold value can appraise
				if ( val < 0.1 )
				{
					val = 9;
				}
				snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
			}
		}
		else if ( tag == "APPRAISE_WORTHLESS_GLASS" )
		{
			if ( (stats[playernum]->PROFICIENCIES[proficiency] + (statGetPER(stats[playernum], player) * 5)) >= 100 )
			{
				snprintf(buf, sizeof(buf), rawValue.c_str(), language[1314]); // yes
			}
			else
			{
				snprintf(buf, sizeof(buf), rawValue.c_str(), language[1315]); // no
			}
		}
		return buf;
	}
	else if ( proficiency == PRO_LOCKPICKING )
	{
		Sint32 PER = statGetPER(stats[playernum], player);
		if ( tag == "TINKERING_LOCKPICK_CHESTS_DOORS" )
		{
			val = stats[playernum]->PROFICIENCIES[proficiency] / 2.f; // lockpick chests/doors
			if ( stats[playernum]->PROFICIENCIES[proficiency] == SKILL_LEVEL_LEGENDARY )
			{
				val = 100.f;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "TINKERING_SCRAP_CHESTS" )
		{
			val = std::min(100.f, stats[playernum]->PROFICIENCIES[proficiency] + 50.f);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "TINKERING_SCRAP_AUTOMATONS" )
		{
			if ( stats[playernum]->PROFICIENCIES[proficiency] >= SKILL_LEVEL_EXPERT )
			{
				val = 100.f; // lockpick automatons
			}
			else
			{
				val = (100 - 100 / (static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20 + 1))); // lockpick automatons
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "TINKERING_DISARM_ARROWS" )
		{
			val = (100 - 100 / (std::max(1, static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 10)))); // disarm arrow traps
			if ( stats[playernum]->PROFICIENCIES[proficiency] < SKILL_LEVEL_BASIC )
			{
				val = 0.f;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "TINKERING_KIT_SCRAP_BONUS" )
		{
			// bonus scrapping chances.
			int skillLVL = std::min(5, static_cast<int>((stats[playernum]->PROFICIENCIES[proficiency] + PER) / 20));
			skillLVL = std::max(0, skillLVL);
			switch ( skillLVL )
			{
				case 5:
					val = 150.f;
					break;
				case 4:
					val = 125.f;
					break;
				case 3:
					val = 50.f;
					break;
				case 2:
					val = 25.f;
					break;
				case 1:
					val = 12.5;
					break;
				default:
					val = 0.f;
					break;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "TINKERING_KIT_REPAIR_ITEM" )
		{
			std::string canRepairItems = language[4057]; // none
			char metalbuf[64] = "";
			char magicbuf[64] = "";
			if ( (stats[playernum]->PROFICIENCIES[proficiency] + PER + (stats[playernum]->type == AUTOMATON ? 20 : 0)) >= SKILL_LEVEL_LEGENDARY )
			{
				canRepairItems = language[4058]; // all
			}
			else if ( (stats[playernum]->PROFICIENCIES[proficiency] + PER + (stats[playernum]->type == AUTOMATON ? 20 : 0)) >= SKILL_LEVEL_MASTER )
			{
				// 2/0
				snprintf(metalbuf, sizeof(metalbuf), language[4059], 2);
				snprintf(magicbuf, sizeof(magicbuf), language[4060], 0);
				canRepairItems = "\x1E ";
				canRepairItems += metalbuf;
				canRepairItems += '\n';
				canRepairItems += "\x1E ";
				canRepairItems += magicbuf;
			}
			else if ( (stats[playernum]->PROFICIENCIES[proficiency] + PER + (stats[playernum]->type == AUTOMATON ? 20 : 0)) >= SKILL_LEVEL_EXPERT )
			{
				// 1/0
				snprintf(metalbuf, sizeof(metalbuf), language[4059], 1);
				snprintf(magicbuf, sizeof(magicbuf), language[4060], 0);
				canRepairItems = "\x1E ";
				canRepairItems += metalbuf;
				canRepairItems += '\n';
				canRepairItems += "\x1E ";
				canRepairItems += magicbuf;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), canRepairItems.c_str());
		}
		else if ( tag == "TINKERING_MAX_ALLIES" )
		{
			val = maximumTinkeringBotsCanBeDeployed(stats[playernum]);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		return buf;
	}
	else if ( proficiency == PRO_ALCHEMY )
	{
		if ( tag == "ALCHEMY_POTION_EFFECT_DMG" )
		{
			int skillLVL = stats[playernum]->PROFICIENCIES[proficiency] / 20;
			// +0% baseline
			val = 100 * (potionDamageSkillMultipliers[std::min(skillLVL, 5)] - potionDamageSkillMultipliers[0]);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "ALCHEMY_THROWN_IMPACT_DMG" )
		{
			int skillLVL = stats[playernum]->PROFICIENCIES[proficiency] / 20;
			// +0% baseline
			val = 100 * (potionDamageSkillMultipliers[std::min(skillLVL, 5)] - potionDamageSkillMultipliers[0]);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "ALCHEMY_DUPLICATION_CHANCE" )
		{
			val = 50.f + static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20) * 10;
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "ALCHEMY_EMPTY_BOTTLE_CONSUME" )
		{
			val = std::min(80, (60 + static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20) * 10));
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "ALCHEMY_EMPTY_BOTTLE_BREW" )
		{
			val = 50.f + static_cast<int>(stats[playernum]->PROFICIENCIES[proficiency] / 20) * 5;
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "ALCHEMY_LEARNT_INGREDIENTS_BASE" )
		{
			std::string outputList = "";
			for ( auto it = clientLearnedAlchemyIngredients[playernum].begin(); 
				it != clientLearnedAlchemyIngredients[playernum].end(); ++it )
			{
				auto alchemyEntry = *it;
				if ( GenericGUI[playernum].isItemBaseIngredient(alchemyEntry) )
				{
					std::string itemName = items[alchemyEntry].name_identified;
					size_t pos = std::string::npos;
					for ( auto& potionName : Player::SkillSheet_t::skillSheetData.potionNamesToFilter )
					{
						if ( (pos = itemName.find(potionName)) != std::string::npos )
						{
							itemName.erase(pos, potionName.length());
						}
					}
					capitalizeString(itemName);
					if ( outputList != "" )
					{
						outputList += '\n';
					}
					outputList += "\x1E " + itemName;
				}
			}
			if ( outputList == "" ) { outputList = "-"; }
			snprintf(buf, sizeof(buf), rawValue.c_str(), outputList.c_str());
		}
		else if ( tag == "ALCHEMY_LEARNT_INGREDIENTS_SECONDARY" )
		{
			std::string outputList = "";
			for ( auto it = clientLearnedAlchemyIngredients[playernum].begin();
				it != clientLearnedAlchemyIngredients[playernum].end(); ++it )
			{
				auto alchemyEntry = *it;
				if ( GenericGUI[playernum].isItemSecondaryIngredient(alchemyEntry)
					&& !GenericGUI[playernum].isItemBaseIngredient(alchemyEntry) )
				{
					std::string itemName = items[alchemyEntry].name_identified;
					size_t pos = std::string::npos;
					for ( auto& potionName : Player::SkillSheet_t::skillSheetData.potionNamesToFilter )
					{
						if ( (pos = itemName.find(potionName)) != std::string::npos )
						{
							itemName.erase(pos, potionName.length());
						}
					}
					if ( outputList != "" )
					{
						outputList += '\n';
					}
					outputList += "\x1E " + itemName;
				}
			}
			if ( outputList == "" ) { outputList = "-"; }
			snprintf(buf, sizeof(buf), rawValue.c_str(), outputList.c_str());
		}
		return buf;
	}
	else if ( proficiency == PRO_SPELLCASTING )
	{
		if ( tag == "CASTING_MP_REGEN" )
		{
			if ( (stats[playernum])->playerRace == RACE_INSECTOID && (stats[playernum])->appearance == 0 )
			{
				return language[4066];
			}
			else if ( (stats[playernum])->type == AUTOMATON )
			{
				return language[4067];
			}
			else
			{
				val = 0.0;
				if ( player )
				{
					val = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				}
				snprintf(buf, sizeof(buf), rawValue.c_str(), val);
			}
		}
		else if ( tag == "CASTING_MP_REGEN_SKILL_MULTIPLIER" )
		{
			if ( (stats[playernum])->playerRace == RACE_INSECTOID && (stats[playernum])->appearance == 0 )
			{
				return language[4066];
			}
			else if ( (stats[playernum])->type == AUTOMATON )
			{
				return language[4067];
			}
			else
			{
				val = 0.0;
				if ( player )
				{
					int skill = stats[playernum]->PROFICIENCIES[proficiency];
					int multiplier = (skill / 20) + 1;
					val = multiplier;
					//real_t normalValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
					//stats[playernum]->PROFICIENCIES[proficiency] = 0;
					//real_t zeroValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
					//stats[playernum]->PROFICIENCIES[proficiency] = skill;
					//
					//val = (100 * zeroValue / normalValue) - 100;
				}
				snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
			}
		}
		else if ( tag == "CASTING_MP_REGEN_SKILL_BONUS" )
		{
			val = 0.0;
			if ( player )
			{
				int skill = stats[playernum]->PROFICIENCIES[proficiency];
				real_t normalValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				stats[playernum]->PROFICIENCIES[proficiency] = 0;
				real_t zeroValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				stats[playernum]->PROFICIENCIES[proficiency] = skill;
				
				val = (100 * zeroValue / normalValue) - 100;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "CASTING_MP_REGEN_BONUS_INT" )
		{
			val = 0.0;
			if ( player )
			{
				int stat = stats[playernum]->INT;
				real_t normalValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				stats[playernum]->INT = 0;
				real_t zeroValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				stats[playernum]->INT = stat;

				val = (100 * zeroValue / normalValue) - 100;
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "CASTING_BEGINNER" )
		{
			if ( isSpellcasterBeginner(playernum, player) )
			{
				snprintf(buf, sizeof(buf), rawValue.c_str(), language[1314]); // yes
			}
			else
			{
				snprintf(buf, sizeof(buf), rawValue.c_str(), language[1315]); // no
			}
		}
		else if ( tag == "CASTING_SPELLBOOK_FUMBLE" )
		{
			int skillLVL = std::min(std::max(0, stats[playernum]->PROFICIENCIES[proficiency] + statGetINT(stats[playernum], player)), 100);
			skillLVL /= 20;
			std::string tierName = language[4061];
			tierName += " ";
			if ( skillLVL <= 0 )
			{
				tierName += "I";
			}
			else if ( skillLVL == 1 )
			{
				tierName += "II";
			}
			else if ( skillLVL == 2 )
			{
				tierName += "III";
			}
			else if ( skillLVL == 3 )
			{
				tierName += "IV";
			}
			else if ( skillLVL == 4 )
			{
				tierName += "V";
			}
			else if ( skillLVL >= 5 )
			{
				tierName += "VI";
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), tierName.c_str());
		}
		return buf;
	}
	else if ( proficiency == PRO_MAGIC )
	{
		if ( tag == "MAGIC_CURRENT_TIER" )
		{
			std::string tierName = language[4061];
			int skillLVL = std::min(stats[playernum]->PROFICIENCIES[proficiency] + statGetINT(stats[playernum], player), 100);
			if ( skillLVL < 0 )
			{
				tierName = language[4057]; // none
			}
			else
			{
				skillLVL /= 20;
				tierName += " ";
				if ( skillLVL == 0 )
				{
					tierName += "I";
				}
				else if ( skillLVL == 1 )
				{
					tierName += "II";
				}
				else if ( skillLVL == 2 )
				{
					tierName += "III";
				}
				else if ( skillLVL == 3 )
				{
					tierName += "IV";
				}
				else if ( skillLVL == 4 )
				{
					tierName += "V";
				}
				else if ( skillLVL >= 5 )
				{
					tierName += "VI";
				}
			}
			snprintf(buf, sizeof(buf), rawValue.c_str(), tierName.c_str());
		}
		else if ( tag == "MAGIC_SPELLPOWER" )
		{
			val = (getBonusFromCasterOfSpellElement(player, stats[playernum]) * 100.0);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "MAGIC_CURRENT_TIER_SPELLS" )
		{
			int skillLVL = std::min(stats[playernum]->PROFICIENCIES[proficiency] + statGetINT(stats[playernum], player), 100);
			if ( skillLVL >= 0 )
			{
				skillLVL /= 20;
			}
			std::string magics = "";
			for ( auto it = allGameSpells.begin(); it != allGameSpells.end(); ++it )
			{
				auto spellEntry = *it;
				if ( spellEntry && spellEntry->difficulty == (skillLVL * 20) )
				{
					if ( magics != "" )
					{
						magics += '\n';
					}
					magics += "\x1E ";
					magics += spellEntry->name;
				}
			}
			if ( magics == "" ) { magics = "-"; }
			snprintf(buf, sizeof(buf), rawValue.c_str(), magics.c_str());
		}
		return buf;
	}
	
	return "";
}

void Player::SkillSheet_t::selectSkill(int skill)
{
	selectedSkill = skill;
	bSkillSheetEntryLoaded = false;
	openTick = ticks;
	resetSkillDisplay();
}

void Player::SkillSheet_t::processSkillSheet()
{
	if ( !skillFrame )
	{
		createSkillSheet();
	}

	// these hardcoded keypresses are evil, WOJ remove this eventually please
	if ( !command && keystatus[SDL_SCANCODE_K] )
	{
		if ( !bSkillSheetOpen )
		{
			openSkillSheet();
		}
		else
		{
			closeSkillSheet();
		}
		keystatus[SDL_SCANCODE_K] = 0;
	}

	if ( !bSkillSheetOpen )
	{
		skillFrame->setDisabled(true);

		auto innerFrame = skillFrame->findFrame("skills frame");
		SDL_Rect pos = innerFrame->getSize();
		skillsFadeInAnimationY = 0.0;
		pos.y = -pos.h;
		innerFrame->setSize(pos);

		auto fade = skillFrame->findImage("fade img");
		Uint8 r, g, b, a;
		SDL_GetRGBA(fade->color, mainsurface->format, &r, &g, &b, &a);
		a = 0;
		fade->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);
		return;
	}

	skillFrame->setDisabled(false);
	skillFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	bool oldCompactViewVal = bUseCompactSkillsView;
	bool oldSlideWindowsOnly = bSlideWindowsOnly;
	if ( splitscreen && 
		((players[player.playernum]->camera_virtualHeight() < Frame::virtualScreenY * .8)
		|| (players[player.playernum]->camera_virtualWidth() < Frame::virtualScreenX * .8)) )
	{
		// use compact view.
		if ( (players[player.playernum]->camera_virtualWidth() < Frame::virtualScreenX * .8)
			&& !(players[player.playernum]->camera_virtualHeight() < Frame::virtualScreenY * .8) )
		{
			bSlideWindowsOnly = true; // 2 player vertical splitscreen, slide but don't use compact view
		}
		else
		{
			bUseCompactSkillsView = true;
		}
	}
	else
	{
		bUseCompactSkillsView = false;
		bSlideWindowsOnly = false;
	}
	if ( (oldCompactViewVal != bUseCompactSkillsView && bUseCompactSkillsView)
		|| (oldSlideWindowsOnly != bSlideWindowsOnly && bSlideWindowsOnly) )
	{
		skillSlideAmount = 1.0;
		skillSlideDirection = 1;
	}
	if ( !bUseCompactSkillsView && !bSlideWindowsOnly )
	{
		skillSlideAmount = 0.0;
		skillSlideDirection = 0;
	}

	auto innerFrame = skillFrame->findFrame("skills frame");
	SDL_Rect sheetSize = innerFrame->getSize();
	Frame* allSkillEntriesLeft = innerFrame->findFrame("skill entries frame left");
	Frame* allSkillEntriesRight = innerFrame->findFrame("skill entries frame right");
	auto leftWingImg = allSkillEntriesLeft->findImage("bg wing left");
	auto rightWingImg = allSkillEntriesRight->findImage("bg wing right");
	auto skillDescriptionFrame = innerFrame->findFrame("skill desc frame");
	auto bgImgFrame = innerFrame->findFrame("skills bg images");
	auto scrollAreaOuterFrame = skillDescriptionFrame->findFrame("scroll area outer frame");
	auto scrollArea = scrollAreaOuterFrame->findFrame("skill scroll area");
	SDL_Rect scrollOuterFramePos = scrollAreaOuterFrame->getSize();
	SDL_Rect scrollAreaPos = scrollArea->getSize();

	real_t slideTravelDistance = 52;
	int compactViewWidthOffset = (skillSlideDirection != 0 ? 104: 0);
	{
		// dynamic width/height adjustments of outer containers
		sheetSize.h = std::max(0, std::min(skillFrame->getSize().h - 8, 
			(int)(404 + (bUseCompactSkillsView ? windowCompactHeightScaleY : windowHeightScaleY) * 80)));
		sheetSize.w = std::max(0, std::min(skillFrame->getSize().w - 8, 
			(int)(684 + (bUseCompactSkillsView ? windowCompactHeightScaleX : windowHeightScaleX) * 80)));
		innerFrame->setSize(sheetSize);

		SDL_Rect skillDescPos = skillDescriptionFrame->getSize();
		skillDescPos.h = innerFrame->getSize().h - skillDescPos.y - 16;
		skillDescPos.w = innerFrame->getSize().w + compactViewWidthOffset - allSkillEntriesLeft->getSize().w - allSkillEntriesRight->getSize().w;
		skillDescPos.x = innerFrame->getSize().w / 2 - skillDescPos.w / 2;
		if ( skillSlideDirection != 0 )
		{
			skillDescPos.x += (skillSlideAmount) * slideTravelDistance;
		}
		skillDescriptionFrame->setSize(skillDescPos);

		scrollOuterFramePos.h = skillDescPos.h - 4;

		auto leftWingPos = allSkillEntriesLeft->getSize();
		auto rightWingPos = allSkillEntriesRight->getSize();

		leftWingPos.x = 0;
		rightWingPos.x = innerFrame->getSize().w - rightWingPos.w;
		leftWingPos.y = std::max(0, innerFrame->getSize().h / 2 - leftWingPos.h / 2);
		rightWingPos.y = std::max(0, innerFrame->getSize().h / 2 - rightWingPos.h / 2);
		allSkillEntriesLeft->setSize(leftWingPos);
		allSkillEntriesRight->setSize(rightWingPos);

		if ( bUseCompactSkillsView )
		{
			leftWingImg->path = "images/ui/SkillSheet/UI_Skills_Window_LeftCompact_03.png";
			rightWingImg->path = "images/ui/SkillSheet/UI_Skills_Window_RightCompact_03.png";
		}
		else
		{
			leftWingImg->path = "images/ui/SkillSheet/UI_Skills_Window_Left_03.png";
			rightWingImg->path = "images/ui/SkillSheet/UI_Skills_Window_Right_03.png";
		}
		leftWingImg->pos.h = Image::get(leftWingImg->path.c_str())->getHeight();
		rightWingImg->pos.h = Image::get(rightWingImg->path.c_str())->getHeight();
	}
	{
		// dynamic main panel width/height background image adjustment
		SDL_Rect bgImgFramePos = bgImgFrame->getSize();
		int backgroundWidth = skillDescriptionFrame->getSize().w;
		int backgroundHeight = innerFrame->getSize().h;

		auto flourishTop = bgImgFrame->findImage("flourish top");
		flourishTop->pos.x = bgImgFramePos.w / 2 - flourishTop->pos.w / 2;
		auto flourishBottom = bgImgFrame->findImage("flourish bottom");
		flourishBottom->pos.x = bgImgFramePos.w / 2 - flourishBottom->pos.w / 2;
		flourishBottom->pos.y = backgroundHeight - flourishBottom->pos.h;

		bgImgFramePos.x = innerFrame->getSize().w / 2 - backgroundWidth / 2;
		if ( skillSlideDirection != 0 )
		{
			bgImgFramePos.x += (skillSlideAmount) * slideTravelDistance;
		}
		bgImgFramePos.y = 0;
		bgImgFramePos.w = backgroundWidth;
		bgImgFramePos.h = backgroundHeight;
		bgImgFrame->setSize(bgImgFramePos);

		imageResizeToContainer9x9(bgImgFrame, SDL_Rect{0, 12, backgroundWidth, backgroundHeight - 6 }, skillsheetEffectBackgroundImages);
	}
	sheetSize.x = skillFrame->getSize().w / 2 - sheetSize.w / 2;

	const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - skillsFadeInAnimationY)) / 5.0;
	skillsFadeInAnimationY += setpointDiffX;
	skillsFadeInAnimationY = std::min(1.0, skillsFadeInAnimationY);

	auto fade = skillFrame->findImage("fade img");
	Uint8 r, g, b, a;
	SDL_GetRGBA(fade->color, mainsurface->format, &r, &g, &b, &a);
	a = 128 * skillsFadeInAnimationY;
	fade->color = SDL_MapRGBA(mainsurface->format, r, g, b, a);

	int baseY = (skillFrame->getSize().h / 2 - sheetSize.h / 2);
	sheetSize.y = -sheetSize.h + skillsFadeInAnimationY * (baseY + sheetSize.h);
	innerFrame->setSize(sheetSize);

	auto slider = skillDescriptionFrame->findSlider("skill slider");
	bool sliderDisabled = slider->isDisabled();

	auto titleText = innerFrame->findField("skill title txt");
	SDL_Rect titleTextPos = titleText->getSize();
	titleTextPos.x = skillDescriptionFrame->getSize().x + skillDescriptionFrame->getSize().w / 2 - titleTextPos.w / 2;
	titleText->setSize(titleTextPos);

	//if ( keystatus[SDL_SCANCODE_F1] )
	//{
	//	if ( keystatus[SDL_SCANCODE_LSHIFT] )
	//	{
	//		windowHeightScaleX = std::max(windowHeightScaleX - .01, -1.0);
	//	}
	//	else
	//	{
	//		windowHeightScaleX = std::min(windowHeightScaleX + .01, 1.0);
	//	}
	//}
	//if ( keystatus[SDL_SCANCODE_F2] )
	//{
	//	if ( keystatus[SDL_SCANCODE_LSHIFT] )
	//	{
	//		windowHeightScaleY = std::max(windowHeightScaleY - .01, -1.0);
	//	}
	//	else
	//	{
	//		windowHeightScaleY = std::min(windowHeightScaleY + .01, 1.0);
	//	}
	//}
	//if ( keystatus[SDL_SCANCODE_F3] )
	//{
	//	keystatus[SDL_SCANCODE_F3] = 0;
	//	if ( keystatus[SDL_SCANCODE_LCTRL] )
	//	{
	//		//bUseCompactSkillsView = !bUseCompactSkillsView;
	//	}
	//	else if ( keystatus[SDL_SCANCODE_LSHIFT] )
	//	{
	//		skillSlideDirection = 0;
	//	}
	//	else
	//	{
	//		skillSlideDirection = (skillSlideDirection != 1) ? 1 : -1;
	//		//skillSlideAmount = 0.0;
	//	}
	//}
	if ( skillSlideDirection != 0 )
	{
		const real_t fpsScale = (144.0 / std::max(1U, fpsLimit)); // ported from 144Hz
		real_t setpointDiff = std::max(0.1, 1.0 - abs(skillSlideAmount));
		skillSlideAmount += fpsScale * (setpointDiff / 5.0) * skillSlideDirection;
		if ( skillSlideAmount < -1.0 )
		{
			skillSlideAmount = -1.0;
		}
		if ( skillSlideAmount > 1.0 )
		{
			skillSlideAmount = 1.0;
		}
	}

	int lowestSkillEntryY = 0;
	bool dpad_moved = false;
	if ( ::inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		highlightedSkill = -1; // if using mouse, clear out the highlighted skill data to be updated below
	}
	int defaultHighlightedSkill = 0;
	if ( selectedSkill >= 0 && selectedSkill < NUMPROFICIENCIES )
	{
		defaultHighlightedSkill = selectedSkill;
	}
	bool closeSheetAction = false;
	if ( player.GUI.activeModule == player.GUI.MODULE_SKILLS_LIST )
	{
		if ( Input::inputs[player.playernum].binaryToggle("MenuUp") )
		{
			dpad_moved = true;
			Input::inputs[player.playernum].consumeBinaryToggle("MenuUp");
			if ( highlightedSkill < 0 || highlightedSkill >= NUMPROFICIENCIES )
			{
				highlightedSkill = defaultHighlightedSkill;
			}
			else if ( highlightedSkill < NUMPROFICIENCIES / 2 )
			{
				--highlightedSkill;
				if ( highlightedSkill < 0 )
				{
					highlightedSkill = (NUMPROFICIENCIES / 2) - 1;
				}
			}
			else
			{
				--highlightedSkill;
				if ( highlightedSkill < NUMPROFICIENCIES / 2 )
				{
					highlightedSkill = NUMPROFICIENCIES - 1;
				}
			}
			if ( selectedSkill != highlightedSkill )
			{
				selectSkill(highlightedSkill);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuDown") )
		{
			dpad_moved = true;
			Input::inputs[player.playernum].consumeBinaryToggle("MenuDown");
			if ( highlightedSkill < 0 || highlightedSkill >= NUMPROFICIENCIES )
			{
				highlightedSkill = defaultHighlightedSkill;
			}
			else if ( highlightedSkill < (NUMPROFICIENCIES / 2) )
			{
				++highlightedSkill;
				if ( highlightedSkill >= NUMPROFICIENCIES / 2 )
				{
					highlightedSkill = 0;
				}
			}
			else
			{
				++highlightedSkill;
				if ( highlightedSkill >= NUMPROFICIENCIES )
				{
					highlightedSkill = (NUMPROFICIENCIES / 2);
				}
			}
			if ( selectedSkill != highlightedSkill )
			{
				selectSkill(highlightedSkill);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuLeft") )
		{
			dpad_moved = true;
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeft");
			if ( highlightedSkill < 0 || highlightedSkill >= NUMPROFICIENCIES )
			{
				highlightedSkill = defaultHighlightedSkill;
			}
			else if ( highlightedSkill < NUMPROFICIENCIES / 2 )
			{
				highlightedSkill += NUMPROFICIENCIES / 2;
			}
			else
			{
				highlightedSkill -= NUMPROFICIENCIES / 2;
			}
			if ( selectedSkill != highlightedSkill )
			{
				selectSkill(highlightedSkill);
			}
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuRight") )
		{
			dpad_moved = true;
			Input::inputs[player.playernum].consumeBinaryToggle("MenuRight");
			if ( highlightedSkill < 0 || highlightedSkill >= NUMPROFICIENCIES )
			{
				highlightedSkill = defaultHighlightedSkill;
			}
			else if ( highlightedSkill < NUMPROFICIENCIES / 2 )
			{
				highlightedSkill += NUMPROFICIENCIES / 2;
			}
			else
			{
				highlightedSkill -= NUMPROFICIENCIES / 2;
			}
			if ( selectedSkill != highlightedSkill )
			{
				selectSkill(highlightedSkill);
			}
		}

		if ( dpad_moved )
		{
			inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
			closeSheetAction = true;
		}
	}
	bool mouseClickedOutOfBounds = false;
	if ( inputs.bPlayerUsingKeyboardControl(player.playernum) && inputs.bMouseLeft(player.playernum) )
	{
		mouseClickedOutOfBounds = true;
	}

	if ( stats[player.playernum] && skillSheetData.skillEntries.size() > 0 )
	{
		bool skillDescAreaCapturesMouse = bgImgFrame->capturesMouse();
		if ( skillDescAreaCapturesMouse ) { mouseClickedOutOfBounds = false; }
		const int skillEntryStartY = bUseCompactSkillsView ? 28 : 38;
		const int entryHeight = bUseCompactSkillsView ? 36 : 40;
		SDL_Rect entryResizePos{ 0, skillEntryStartY, 0, entryHeight };
		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			char skillname[32];
			snprintf(skillname, sizeof(skillname), "skill %d", i);
			Frame* allSkillEntries = allSkillEntriesRight;
			if ( i >= 8 )
			{
				allSkillEntries = allSkillEntriesLeft;
				if ( i == 8 )
				{
					entryResizePos.y = skillEntryStartY;
				}
			}
			auto entry = allSkillEntries->findFrame(skillname);

			if ( i >= skillSheetData.skillEntries.size() )
			{
				entry->setDisabled(true);
				break;
			}
			entry->setDisabled(false);
			SDL_Rect entryPos = entry->getSize();
			entryPos.y = entryResizePos.y;
			entryPos.h = entryResizePos.h;
			entry->setSize(entryPos);
			entryResizePos.y += entryResizePos.h;
			lowestSkillEntryY = std::max(lowestSkillEntryY, entryPos.y + entryPos.h);

			int proficiency = skillSheetData.skillEntries[i].skillId;

			auto skillLevel = entry->findField("skill level");
			char skillLevelText[32];
			snprintf(skillLevelText, sizeof(skillLevelText), "%d", stats[player.playernum]->PROFICIENCIES[proficiency]);
			skillLevel->setText(skillLevelText);

			auto skillIconBg = entry->findImage("skill icon bg");
			auto skillIconFg = entry->findImage("skill icon fg");
			skillIconFg->path = skillSheetData.skillEntries[i].skillIconPath;

			auto statIcon = entry->findImage("stat icon");
			statIcon->disabled = true;

			auto selectorIcon = entry->findImage("selector img");
			selectorIcon->disabled = true;

			if ( entry->capturesMouse() )
			{
				mouseClickedOutOfBounds = false;
				if ( ::inputs.getVirtualMouse(player.playernum)->draw_cursor
					&& entry->capturesMouse() && !skillDescAreaCapturesMouse )
				{
					highlightedSkill = i;
					if ( skillSlideDirection != 0 )
					{
						if ( highlightedSkill >= 8 )
						{
							skillSlideDirection = 1;
						}
						else
						{
							skillSlideDirection = -1;
						}
					}
					if ( inputs.bMouseLeft(player.playernum) )
					{
						selectSkill(i);
						inputs.mouseClearLeft(player.playernum);
					}
				}
			}

			if ( selectedSkill == i || highlightedSkill == i )
			{
				selectorIcon->disabled = false;
				if ( selectedSkill == i && highlightedSkill == i )
				{
					selectorIcon->path = (i < 8) ? "images/ui/SkillSheet/UI_Skills_SkillSelector100_01.png" : "images/ui/SkillSheet/UI_Skills_SkillSelector100R_01.png";
				}
				else if ( highlightedSkill == i )
				{
					selectorIcon->path = (i < 8) ? skillSheetData.highlightSkillImg : skillSheetData.highlightSkillImg_Right;
				}
				else if ( selectedSkill == i )
				{
					selectorIcon->path = (i < 8) ? skillSheetData.selectSkillImg : skillSheetData.selectSkillImg_Right;
				}
			}

			if ( stats[player.playernum]->PROFICIENCIES[proficiency] >= SKILL_LEVEL_LEGENDARY )
			{
				skillIconFg->path = skillSheetData.skillEntries[i].skillIconPathLegend;
				skillLevel->setColor(skillSheetData.legendTextColor);
				if ( selectedSkill == i )
				{
					skillIconBg->path = skillSheetData.iconBgSelectedPathLegend;
				}
				else
				{
					skillIconBg->path = skillSheetData.iconBgPathLegend;
				}
			}
			else if ( stats[player.playernum]->PROFICIENCIES[proficiency] >= SKILL_LEVEL_EXPERT )
			{
				skillLevel->setColor(skillSheetData.expertTextColor);
				if ( selectedSkill == i )
				{
					skillIconBg->path = skillSheetData.iconBgSelectedPathExpert;
				}
				else
				{
					skillIconBg->path = skillSheetData.iconBgPathExpert;
				}
			}
			else if ( stats[player.playernum]->PROFICIENCIES[proficiency] >= SKILL_LEVEL_BASIC )
			{
				skillLevel->setColor(skillSheetData.noviceTextColor);
				if ( selectedSkill == i )
				{
					skillIconBg->path = skillSheetData.iconBgSelectedPathNovice;
				}
				else
				{
					skillIconBg->path = skillSheetData.iconBgPathNovice;
				}
			}
			else
			{
				skillLevel->setColor(skillSheetData.defaultTextColor);
				if ( selectedSkill == i )
				{
					skillIconBg->path = skillSheetData.iconBgSelectedPathDefault;
				}
				else
				{
					skillIconBg->path = skillSheetData.iconBgPathDefault;
				}
			}
			//statIcon->path = skillIconFg->path;
			//skillIconFg->path = "";
		}

		int lowestY = 0;

		/*if ( keystatus[SDL_SCANCODE_J] )
		{
			keystatus[SDL_SCANCODE_J] = 0;
			slider->setDisabled(!slider->isDisabled());
		}*/
		SDL_Rect sliderPos = slider->getRailSize();
		sliderPos.x = skillDescriptionFrame->getSize().w - 34;
		sliderPos.h = skillDescriptionFrame->getSize().h - 8;
		slider->setRailSize(sliderPos);

		int sliderOffsetW = 0;
		if ( slider->isDisabled() )
		{
			int diff = (scrollOuterFramePos.w - (skillDescriptionFrame->getSize().w - 32));
			sliderOffsetW = -diff;
		}
		else
		{
			int diff = (scrollOuterFramePos.w - (slider->getRailSize().x - 4 - 16));
			sliderOffsetW = -diff;
		}
		if ( sliderOffsetW != 0 )
		{
			bSkillSheetEntryLoaded = false;
		}

		scrollOuterFramePos.w += sliderOffsetW;
		scrollAreaOuterFrame->setSize(scrollOuterFramePos);
		scrollAreaPos.w = scrollOuterFramePos.w;
		scrollArea->setSize(scrollAreaPos);

		if ( slider->isDisabled() )
		{
			scrollInertia = 0.0;
		}

		if ( selectedSkill >= 0 && selectedSkill < skillSheetData.skillEntries.size() )
		{
			auto skillLvlHeaderVal = scrollArea->findField("skill lvl header val");
			char skillLvl[128] = "";
			int proficiency = skillSheetData.skillEntries[selectedSkill].skillId;
			int proficiencyValue = stats[player.playernum]->PROFICIENCIES[proficiency];
			std::string skillLvlTitle = "";
			if ( proficiencyValue >= SKILL_LEVEL_LEGENDARY )
			{
				skillLvlTitle = language[369];
			}
			else if ( proficiencyValue >= SKILL_LEVEL_MASTER )
			{
				skillLvlTitle = language[368];
			}
			else if ( proficiencyValue >= SKILL_LEVEL_EXPERT )
			{
				skillLvlTitle = language[367];
			}
			else if ( proficiencyValue >= SKILL_LEVEL_SKILLED )
			{
				skillLvlTitle = language[366];
			}
			else if ( proficiencyValue >= SKILL_LEVEL_BASIC )
			{
				skillLvlTitle = language[365];
			}
			else if ( proficiencyValue >= SKILL_LEVEL_NOVICE )
			{
				skillLvlTitle = language[364];
			}
			else
			{
				skillLvlTitle = language[363];
			}
			skillLvlTitle.erase(std::remove(skillLvlTitle.begin(), skillLvlTitle.end(), ' '), skillLvlTitle.end()); // trim whitespace
			snprintf(skillLvl, sizeof(skillLvl), "%s (%d)", skillLvlTitle.c_str(), stats[player.playernum]->PROFICIENCIES[proficiency]);
			skillLvlHeaderVal->setText(skillLvl);

			SDL_Rect skillLvlHeaderValPos = skillLvlHeaderVal->getSize();
			skillLvlHeaderValPos.x += sliderOffsetW;
			skillLvlHeaderVal->setSize(skillLvlHeaderValPos);

			if ( !bSkillSheetEntryLoaded )
			{
				auto skillTitleTxt = innerFrame->findField("skill title txt");
				skillTitleTxt->setText(skillSheetData.skillEntries[selectedSkill].name.c_str());

				auto statTypeTxt = scrollArea->findField("stat type txt");
				auto statIcon = scrollArea->findImage("stat icon");
				statIcon->path = skillSheetData.skillEntries[selectedSkill].statIconPath;
				switch ( getStatForProficiency(proficiency) )
				{
					case STAT_STR:
						statTypeTxt->setText("STR");
						break;
					case STAT_DEX:
						statTypeTxt->setText("DEX");
						break;
					case STAT_CON:
						statTypeTxt->setText("CON");
						break;
					case STAT_INT:
						statTypeTxt->setText("INT");
						break;
					case STAT_PER:
						statTypeTxt->setText("PER");
						break;
					case STAT_CHR:
						statTypeTxt->setText("CHR");
						break;
					default:
						break;
				}

				SDL_Rect statTypeTxtPos = statTypeTxt->getSize();
				statTypeTxtPos.x += sliderOffsetW;
				statTypeTxt->setSize(statTypeTxtPos);
				statIcon->pos.x += sliderOffsetW;
			}

			auto skillDescriptionTxt = scrollArea->findField("skill desc txt");
			auto skillDescriptionBgFrame = scrollArea->findFrame("skill desc bg frame");
			int moveEffectsOffsetY = 0;
			Font* actualFont = Font::get(skillDescriptionTxt->getFont());
			if ( !bSkillSheetEntryLoaded )
			{
				SDL_Rect skillDescriptionTxtPos = skillDescriptionTxt->getSize();
				skillDescriptionTxtPos.w = scrollAreaPos.w;
				skillDescriptionTxt->setSize(skillDescriptionTxtPos);

				int txtHeightOld = skillDescriptionTxt->getNumTextLines() * actualFont->height(true);
				skillDescriptionTxt->setText(skillSheetData.skillEntries[selectedSkill].description.c_str());
				skillDescriptionTxt->reflowTextToFit(0);
				int txtHeightNew = skillDescriptionTxt->getNumTextLines() * actualFont->height(true);

				if ( txtHeightNew != txtHeightOld )
				{
					moveEffectsOffsetY = (txtHeightNew - txtHeightOld);
				}
				skillDescriptionBgFrame->setSize(SDL_Rect{skillDescriptionTxtPos.x, skillDescriptionTxtPos.y - 4,
					skillDescriptionTxtPos.w, txtHeightNew + 8 });
				imageResizeToContainer9x9(skillDescriptionBgFrame, 
					SDL_Rect{ 0, 0, skillDescriptionBgFrame->getSize().w, skillDescriptionBgFrame->getSize().h }, skillsheetEffectBackgroundImages);
			}

			lowestY = std::max(lowestY, skillDescriptionTxt->getSize().y + skillDescriptionTxt->getNumTextLines() * actualFont->height(true));
			
			int previousEffectFrameHeight = 0;
			for ( int eff = 0; eff < 10; ++eff )
			{
				char effectFrameName[64] = "";
				snprintf(effectFrameName, sizeof(effectFrameName), "effect %d frame", eff);
				auto effectFrame = scrollArea->findFrame(effectFrameName);
				if ( !effectFrame ) { continue; }

				effectFrame->setDisabled(true);
				SDL_Rect effectFramePos = effectFrame->getSize();
				effectFramePos.w = scrollAreaPos.w;

				if ( moveEffectsOffsetY != 0 )
				{
					effectFramePos.y += moveEffectsOffsetY;
				}
				effectFrame->setSize(effectFramePos);

				if ( eff < skillSheetData.skillEntries[selectedSkill].effects.size() )
				{
					effectFrame->setDisabled(false);

					auto& effect_t = skillSheetData.skillEntries[selectedSkill].effects[eff];
					auto effectTxtFrame = effectFrame->findFrame("effect txt frame");
					auto effectTxt = effectTxtFrame->findField("effect txt");
					auto effectValFrame = effectFrame->findFrame("effect val frame");
					auto effectVal = effectValFrame->findField("effect val");
					auto effectBgImgFrame = effectFrame->findFrame("effect val bg frame");

					effectTxt->setText(effect_t.title.c_str());
					if ( effect_t.bAllowRealtimeUpdate 
						|| effect_t.effectUpdatedAtSkillLevel != stats[player.playernum]->PROFICIENCIES[proficiency]
						|| effect_t.value == "" )
					{
						effect_t.effectUpdatedAtSkillLevel = stats[player.playernum]->PROFICIENCIES[proficiency];
						effect_t.value = formatSkillSheetEffects(player.playernum, proficiency, effect_t.tag, effect_t.rawValue);
					}
					effectVal->setText(effect_t.value.c_str());
					auto textGetValue = Text::get(effectVal->getLongestLine().c_str(), effectVal->getFont(),
						effectVal->getTextColor(), effectVal->getOutlineColor());
					const int valueWidth = textGetValue->getWidth();

					{
						// adjust position to match width of container
						SDL_Rect effectTxtFramePos = effectTxtFrame->getSize();
						SDL_Rect effectValFramePos = effectValFrame->getSize();
						SDL_Rect effectBgImgFramePos = effectBgImgFrame->getSize();
						const auto& effectStartOffsetX = skillSheetData.skillEntries[selectedSkill].effectStartOffsetX;
						const auto& effectBackgroundOffsetX = skillSheetData.skillEntries[selectedSkill].effectBackgroundOffsetX;
						const auto& effectBackgroundWidth = skillSheetData.skillEntries[selectedSkill].effectBackgroundWidth;
						int valueCustomWidthOffset = effect_t.valueCustomWidthOffset;
						effectBgImgFramePos.x = effectFrame->getSize().w - effectStartOffsetX - effectBackgroundOffsetX - valueCustomWidthOffset;
						effectBgImgFramePos.w = effectBackgroundWidth + valueCustomWidthOffset;
						effectTxtFramePos.w = effectFrame->getSize().w - effectStartOffsetX - effectBackgroundOffsetX - valueCustomWidthOffset;
						effectValFramePos.x = effectFrame->getSize().w - effectStartOffsetX - valueCustomWidthOffset;
						effectValFramePos.w = effectStartOffsetX + valueCustomWidthOffset;
						if ( effect_t.bAllowAutoResizeValue 
							&& valueWidth > (effectValFramePos.w - effectVal->getSize().x - effectBackgroundOffsetX) )
						{
							int diff = (valueWidth - (effectValFramePos.w - effectVal->getSize().x - effectBackgroundOffsetX));
							effectBgImgFramePos.x -= diff;
							effectBgImgFramePos.w += diff;
							effectTxtFramePos.w -= diff;
							effectValFramePos.x -= diff;
							effectValFramePos.w += diff;
						}

						effectTxtFrame->setSize(effectTxtFramePos);
						effectValFrame->setSize(effectValFramePos);
						effectBgImgFrame->setSize(effectBgImgFramePos);
					}

					Font* effectTxtFont = Font::get(effectTxt->getFont());
					int fontHeight;
					effectTxtFont->sizeText("_", nullptr, &fontHeight);
					int numEffectLines = effectTxt->getNumTextLines();
					int numEffectValLines = effectVal->getNumTextLines();
					int numEffectValBgLines = numEffectValLines;
					effectFramePos = effectFrame->getSize();
					if ( numEffectLines > 1 || numEffectValLines > 1 )
					{
						if ( numEffectValLines <= 1 )
						{
							// single line value, only need lines of title
							effectFramePos.h = (fontHeight * std::max(1, numEffectLines)) + 8;
						}
						else
						{
							if ( numEffectLines <= numEffectValLines )
							{
								numEffectValLines += 1; // need more buffer area for the values as it is larger than title
								effectFramePos.h = (fontHeight * numEffectValLines) + 8;
							}
							else
							{
								effectFramePos.h = (fontHeight * numEffectLines) + 8;
							}
						}
					}
					else
					{
						// both title and value are 1 line, add .5 padding
						effectFramePos.h = (fontHeight) * 1.5 + 8;
					}
					if ( eff > 0 )
					{
						effectFramePos.y = previousEffectFrameHeight; // don't adjust first effect frame y pos
					}
					effectFrame->setSize(effectFramePos);

					{
						// adjust position to match height of container
						SDL_Rect effectTxtFramePos = effectTxtFrame->getSize();
						SDL_Rect effectValFramePos = effectValFrame->getSize();
						SDL_Rect effectBgImgFramePos = effectBgImgFrame->getSize();
						const int containerHeight = effectFramePos.h - 4;
						effectTxtFramePos.h = containerHeight;
						effectValFramePos.h = containerHeight;
						effectTxtFrame->setSize(effectTxtFramePos);
						effectValFrame->setSize(effectValFramePos);
	
						SDL_Rect effectTxtPos = effectTxt->getSize();
						effectTxtPos.h = containerHeight;
						effectTxt->setSize(effectTxtPos);
						SDL_Rect effectValPos = effectVal->getSize();
						effectValPos.h = containerHeight;
						effectVal->setSize(effectValPos);

						effectBgImgFramePos.h = (fontHeight * numEffectValBgLines) + 8;
						effectBgImgFramePos.y = (containerHeight / 2 - effectBgImgFramePos.h / 2);
						effectBgImgFrame->setSize(effectBgImgFramePos);

						auto effectFrameBgImg = effectFrame->findImage("effect frame bg highlight");
						effectFrameBgImg->pos = SDL_Rect{ 0, effectFrame->getSize().h - 2, effectFrame->getSize().w, 1 };

						auto effectFrameBgImgTmp = effectFrame->findImage("effect frame bg tmp");
						effectFrameBgImgTmp->pos = SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h };
					}

					{
						// adjust inner background image elements
						imageResizeToContainer9x9(effectBgImgFrame, 
							SDL_Rect{ 0, 0, effectBgImgFrame->getSize().w, effectBgImgFrame->getSize().h }, skillsheetEffectBackgroundImages);
					}

					lowestY = std::max(lowestY, effectFrame->getSize().y + effectFrame->getSize().h);

					auto textGetTitle = Text::get(effectTxt->getText(), effectTxt->getFont(),
						effectTxt->getTextColor(), effectTxt->getOutlineColor());
					int titleWidth = textGetTitle->getWidth();
					if ( numEffectLines > 1 )
					{
						auto textGetTitle = Text::get(effectTxt->getLongestLine().c_str(), effectTxt->getFont(),
							effectTxt->getTextColor(), effectTxt->getOutlineColor());
						titleWidth = textGetTitle->getWidth();
					}

					// check marquee if needed
					if ( ticks - openTick > TICKS_PER_SECOND * 2 )
					{
						bool doMarquee = false;
						doMarquee = doMarquee || (titleWidth > (effectTxt->getSize().x + effectTxtFrame->getSize().w));
						doMarquee = doMarquee || (valueWidth > (effectVal->getSize().x + effectValFrame->getSize().w));

						if ( doMarquee )
						{
							const real_t fpsScale = (60.f / std::max(1U, fpsLimit)); // ported from 60Hz
							effect_t.marquee += (.005 * fpsScale);
							//effect_t.marquee = std::min(1.0, effect_t.marquee);

							/*if ( effect_t.marqueeTicks == 0 && effect_t.marquee >= 1.0 )
							{
								effect_t.marqueeTicks = ticks;
							}*/
							/*if ( effect_t.marqueeTicks > 0 && (ticks - effect_t.marqueeTicks > TICKS_PER_SECOND * 2) )
							{
								effect_t.marqueeTicks = 0;
								effect_t.marquee = 0.0;
							}*/
						}
					}
					SDL_Rect posTitle = effectTxt->getSize();
					int scrollTitleLength = titleWidth - effectTxtFrame->getSize().w;
					if ( titleWidth <= effectTxtFrame->getSize().w )
					{
						scrollTitleLength = 0;
						posTitle.x = 0;
						effect_t.marqueeCompleted = false;
						effect_t.marquee = 0.0;
						effect_t.marqueeTicks = 0;
					}
					else
					{
						posTitle.x = std::max((int)(-effect_t.marquee * 100), -scrollTitleLength);
						if ( posTitle.x == -scrollTitleLength )
						{
							if ( !effect_t.marqueeCompleted )
							{
								effect_t.marqueeTicks = ticks;
							}
							effect_t.marqueeCompleted = true;
						}
						else
						{
							effect_t.marqueeCompleted = false;
						}
					}
					//posTitle.x = -scrollTitleLength * effect_t.marquee;
					effectTxt->setSize(posTitle);

					SDL_Rect posValue = effectVal->getSize();
					int scrollValueLength = valueWidth - effectValFrame->getSize().w;
					if ( valueWidth <= effectValFrame->getSize().w )
					{
						scrollValueLength = 0;
					}
					posValue.x = -scrollValueLength * effect_t.marquee;
					effectVal->setSize(posValue);
				}

				previousEffectFrameHeight = effectFrame->getSize().y + effectFrame->getSize().h;
			}

			Uint32 lastMarqueeTick = 0;
			bool allMarqueeCompleted = true;
			for ( auto& effect_t : skillSheetData.skillEntries[selectedSkill].effects )
			{
				if ( effect_t.marquee > 0.0 )
				{
					if ( !effect_t.marqueeCompleted )
					{
						allMarqueeCompleted = false;
					}
					lastMarqueeTick = std::max(effect_t.marqueeTicks, lastMarqueeTick);
				}
			}
			if ( allMarqueeCompleted && lastMarqueeTick > 0 && ((ticks - lastMarqueeTick) > 2 * TICKS_PER_SECOND) )
			{
				for ( auto& effect_t : skillSheetData.skillEntries[selectedSkill].effects )
				{
					effect_t.marquee = 0.0;
					effect_t.marqueeCompleted = false;
					effect_t.marqueeTicks = 0;
				}
				openTick = (ticks > TICKS_PER_SECOND) ? (ticks - TICKS_PER_SECOND) : ticks;
			}

			Uint32 legendGoldColor = makeColor(230, 183, 20, 255);
			Uint32 legendRegularColor = makeColor(201, 162, 100, 255);

			// legend panel
			auto legendDivImg = scrollArea->findImage("legend div");
			legendDivImg->pos.x = scrollArea->getSize().w / 2 - legendDivImg->pos.w / 2;
			legendDivImg->pos.y = lowestY + 8;
			auto legendDivTxt = scrollArea->findField("legend div text");
			SDL_Rect legendDivTxtPos = legendDivTxt->getSize();
			legendDivTxtPos.x = legendDivImg->pos.x;
			legendDivTxtPos.w = legendDivImg->pos.w;
			legendDivTxtPos.y = legendDivImg->pos.y + legendDivImg->pos.h - 16;
			legendDivTxtPos.h = actualFont->height(true);
			legendDivTxt->setSize(legendDivTxtPos);
			lowestY = legendDivTxtPos.y + legendDivTxtPos.h;

			auto legendBackerImg = scrollArea->findImage("legend txt backer img");
			legendBackerImg->pos.x = scrollArea->getSize().w / 2 - legendBackerImg->pos.w / 2;
			legendBackerImg->pos.y = legendDivTxtPos.y + 6;
			legendBackerImg->disabled = true;
			if ( proficiencyValue >= SKILL_LEVEL_LEGENDARY )
			{
				legendBackerImg->disabled = false;
			}

			auto legendFrame = scrollArea->findFrame("legend frame");
			SDL_Rect legendPos = legendFrame->getSize();
			legendPos.y = lowestY;
			legendPos.w = scrollArea->getSize().w;
			legendFrame->setSize(legendPos);

			auto tl = legendFrame->findImage("top left img");
			auto tm = legendFrame->findImage("top img");
			auto tr = legendFrame->findImage("top right img");
			tm->pos.w = legendPos.w - tl->pos.w - tr->pos.w;
			tr->pos.x = legendPos.w - tr->pos.w;

			auto legendText = legendFrame->findField("legend text");
			legendText->setText(skillSheetData.skillEntries[selectedSkill].legendaryDescription.c_str());
			SDL_Rect legendTextPos = legendText->getSize();
			legendTextPos.w = tm->pos.w;
			legendText->setSize(legendTextPos);
			legendText->reflowTextToFit(0);
			legendTextPos.h = legendText->getNumTextLines() * actualFont->height(true);
			legendTextPos.y = tm->pos.y + tm->pos.h / 2;
			legendText->setSize(legendTextPos);

			auto ml = legendFrame->findImage("middle left img");
			ml->pos.h = legendTextPos.h - tm->pos.h;
			auto mm = legendFrame->findImage("middle img");
			mm->pos.h = ml->pos.h;
			auto mr = legendFrame->findImage("middle right img");
			mr->pos.h = ml->pos.h;
			mm->pos.w = legendPos.w - ml->pos.w - mr->pos.w;
			mr->pos.x = legendPos.w - mr->pos.w;

			auto bl = legendFrame->findImage("bottom left img");
			bl->pos.y = ml->pos.y + ml->pos.h;
			auto bm = legendFrame->findImage("bottom img");
			bm->pos.y = bl->pos.y;
			auto br = legendFrame->findImage("bottom right img");
			br->pos.y = bl->pos.y;
			bm->pos.w = legendPos.w - bl->pos.w - br->pos.w;
			br->pos.x = legendPos.w - br->pos.w;

			if ( proficiencyValue < SKILL_LEVEL_LEGENDARY )
			{
				legendDivTxt->setColor(legendRegularColor);
				legendText->setColor(legendRegularColor);
				tl->path = "images/ui/SkillSheet/UI_Skills_LegendBox_TL_00.png";
				tm->path = "images/ui/SkillSheet/UI_Skills_LegendBox_T_00.png";
				tr->path = "images/ui/SkillSheet/UI_Skills_LegendBox_TR_00.png";

				ml->path = "images/ui/SkillSheet/UI_Skills_LegendBox_ML_00.png";
				mm->path = "images/ui/SkillSheet/UI_Skills_LegendBox_M_00.png";
				mr->path = "images/ui/SkillSheet/UI_Skills_LegendBox_MR_00.png";

				bl->path = "images/ui/SkillSheet/UI_Skills_LegendBox_BL_00.png";
				bm->path = "images/ui/SkillSheet/UI_Skills_LegendBox_B_00.png";
				br->path = "images/ui/SkillSheet/UI_Skills_LegendBox_BR_00.png";
			}
			else
			{
				legendDivTxt->setColor(legendGoldColor);
				legendText->setColor(legendGoldColor);
				tl->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_TL_00.png";
				tm->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_T_00.png";
				tr->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_TR_00.png";

				ml->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_ML_00.png";
				mm->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_M_00.png";
				mr->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_MR_00.png";

				bl->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_BL_00.png";
				bm->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_B_00.png";
				br->path = "images/ui/SkillSheet/UI_Skills_LegendBox100_BR_00.png";
			}

			legendPos.h = bl->pos.y + bl->pos.h;
			legendFrame->setSize(legendPos);

			lowestY = legendPos.y + legendPos.h;
			if ( lowestY < scrollAreaOuterFrame->getSize().h )
			{
				// shift the legend items as far to the bottom as possible.
				int offset = scrollAreaOuterFrame->getSize().h - lowestY;
				legendDivImg->pos.y += offset;
				legendBackerImg->pos.y += offset;
				legendDivTxt->setPos(legendDivTxt->getSize().x, legendDivTxt->getSize().y + offset);
				legendFrame->setPos(legendFrame->getSize().x, legendFrame->getSize().y + offset);
			}
			//lowestY += 4; // small buffer after legend box
		}

		if ( !slider->isDisabled() )
		{
			if ( inputs.bPlayerUsingKeyboardControl(player.playernum) )
			{
				if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
				{
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					scrollInertia = std::min(scrollInertia + .05, .15);
				}
				if ( mousestatus[SDL_BUTTON_WHEELUP] )
				{
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
					scrollInertia = std::max(scrollInertia - .05, -.15);
				}
			}
			if ( Input::inputs[player.playernum].analog("MenuScrollDown") )
			{
				scrollInertia = 0.0;
				real_t delta = Input::inputs[player.playernum].analog("MenuScrollDown");
				scrollPercent = std::min(1.0, scrollPercent + .05 * (60.f / std::max(1U, fpsLimit)) * delta);
				slider->setValue(scrollPercent * 100);
			}
			else if ( Input::inputs[player.playernum].analog("MenuScrollUp") )
			{
				scrollInertia = 0.0;
				real_t delta = Input::inputs[player.playernum].analog("MenuScrollUp");
				scrollPercent = std::max(0.0, scrollPercent -.05 * (60.f / std::max(1U, fpsLimit) * delta));
				slider->setValue(scrollPercent * 100);
			}
		}

		if ( abs(scrollInertia) > 0.0 )
		{
			scrollInertia *= .9;
			if ( abs(scrollInertia) < .01 )
			{
				scrollInertia = 0.0;
			}
			scrollPercent = std::min(1.0, std::max(scrollPercent + scrollInertia, 0.00));
			if ( scrollPercent >= 1.0 || scrollPercent <= 0.0 )
			{
				scrollInertia = 0.0;
			}
			slider->setValue(scrollPercent * 100);
		}

		scrollPercent = slider->getValue() / 100.0;
		scrollAreaPos = scrollArea->getSize();
		if ( lowestY > scrollAreaOuterFrame->getSize().h )
		{
			scrollAreaPos.y = -(lowestY - scrollAreaOuterFrame->getSize().h) * scrollPercent;
			slider->setDisabled(false);
		}
		else
		{
			scrollAreaPos.y = 0;
			slider->setDisabled(true);
		}
		scrollArea->setSize(scrollAreaPos);
	}

	bool drawGlyphs = !::inputs.getVirtualMouse(player.playernum)->draw_cursor;
	if ( auto promptBack = skillFrame->findField("prompt back txt") )
	{
		promptBack->setDisabled(!drawGlyphs);
		promptBack->setText(language[4053]);
		auto promptImg = skillFrame->findImage("prompt back img");
		promptImg->disabled = !drawGlyphs;
		SDL_Rect glyphPos = promptImg->pos;
		if ( auto textGet = Text::get(promptBack->getText(), promptBack->getFont(),
			promptBack->getTextColor(), promptBack->getOutlineColor()) )
		{
			SDL_Rect textPos = promptBack->getSize();
			textPos.w = textGet->getWidth();
			textPos.x = innerFrame->getSize().x + innerFrame->getSize().w - textPos.w - 16;
			textPos.y = innerFrame->getSize().y + allSkillEntriesLeft->getSize().y + lowestSkillEntryY + 4;
			if ( !bUseCompactSkillsView )
			{
				textPos.y -= 4;
			}
			promptBack->setSize(textPos);
			glyphPos.x = promptBack->getSize().x + promptBack->getSize().w - textGet->getWidth() - 4;
		}
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForInput("MenuCancel");
		Image* glyphImage = Image::get(promptImg->path.c_str());
		if ( glyphImage )
		{
			glyphPos.w = glyphImage->getWidth();
			glyphPos.h = glyphImage->getHeight();
			glyphPos.x -= glyphPos.w;
			glyphPos.y = promptBack->getSize().y + promptBack->getSize().h / 2 - glyphPos.h / 2;
			promptImg->pos = glyphPos;
		}
	}
	if ( auto promptScroll = skillFrame->findField("prompt scroll txt") )
	{
		promptScroll->setDisabled(!drawGlyphs);
		if ( bUseCompactSkillsView )
		{
			promptScroll->setText(language[4063]);
		}
		else
		{
			promptScroll->setText(language[4062]);
		}
		auto promptImg = skillFrame->findImage("prompt scroll img");
		promptImg->disabled = !drawGlyphs;
		SDL_Rect glyphPos = promptImg->pos;
		if ( auto textGet = Text::get(promptScroll->getText(), promptScroll->getFont(),
			promptScroll->getTextColor(), promptScroll->getOutlineColor()) )
		{
			SDL_Rect textPos = promptScroll->getSize();
			textPos.w = textGet->getWidth();
			if ( bUseCompactSkillsView )
			{
				textPos.x = innerFrame->getSize().x + 16;
				textPos.y = innerFrame->getSize().y + allSkillEntriesLeft->getSize().y + lowestSkillEntryY + 4;
			}
			else
			{
				textPos.x = innerFrame->getSize().x + innerFrame->getSize().w / 2 - textPos.w / 2;
				textPos.y = innerFrame->getSize().y + innerFrame->getSize().h - 8;
			}
			promptScroll->setSize(textPos);
		}
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForInput("MenuScrollDown");
		Image* glyphImage = Image::get(promptImg->path.c_str());
		if ( glyphImage )
		{
			SDL_Rect textPos = promptScroll->getSize();
			glyphPos.w = glyphImage->getWidth();
			glyphPos.h = glyphImage->getHeight();
			glyphPos.x = textPos.x;
			if ( !bUseCompactSkillsView ) // centred
			{
				glyphPos.x -= glyphPos.w / 2 + 2;
			}
			glyphPos.y = promptScroll->getSize().y + promptScroll->getSize().h / 2 - glyphPos.h / 2;
			promptImg->pos = glyphPos;

			textPos.x = glyphPos.x + glyphPos.w + 4;
			promptScroll->setSize(textPos);
		}
	}

	if ( closeSheetAction || mouseClickedOutOfBounds )
	{
		if ( mouseClickedOutOfBounds )
		{
			inputs.mouseClearLeft(player.playernum);
		}
		closeSkillSheet();
		return;
	}

	if ( sliderDisabled != slider->isDisabled() )
	{
		// rerun this function
		processSkillSheet();
		bSkillSheetEntryLoaded = false;
	}
	else
	{
		bSkillSheetEntryLoaded = true;
	}

}