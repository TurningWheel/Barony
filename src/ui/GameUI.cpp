// GameUI.cpp

#include "GameUI.hpp"
#include "MainMenu.hpp"
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
#include "../interface/consolecommand.hpp"
#include "../stat.hpp"
#include "../player.hpp"
#include "../draw.hpp"
#include "../items.hpp"
#include "../mod_tools.hpp"
#include "../input.hpp"
#include "../collision.hpp"
#include "../monster.hpp"
#include "../classdescriptions.hpp"
#include "../shops.hpp"
#include "../colors.hpp"

#include <assert.h>

static const char* bigfont_outline = "fonts/pixelmix.ttf#16#2";
static const char* bigfont_no_outline = "fonts/pixelmix.ttf#16#0";
static const char* smallfont_outline = "fonts/pixel_maz_multiline.ttf#16#2";
static const char* smallfont_no_outline = "fonts/pixel_maz_multiline.ttf#16#0";

static ConsoleVariable<Vector4> mapBgColor("/map_background_color", Vector4{22.f, 24.f, 29.f, 223.f});
static ConsoleVariable<Vector4> logBgColor("/log_background_color", Vector4{22.f, 24.f, 29.f, 223.f});

Frame* gameUIFrame[MAXPLAYERS] = { nullptr };
bool newui = true;
int selectedCursorOpacity = 255;
int oldSelectedCursorOpacity = 255;
int hotbarSlotOpacity = 255;
int hotbarSelectedSlotOpacity = 255;
int hotbarCompactOffsetX = 0;
real_t hotbarCompactSlotOverlapPercent = 0.0; // % of a slot to overlap facemenu buttons in compact view
int hotbarCompactInactiveSlotMovementX = 0; // inactive facemenu groups move over by this much out of the way in compact view
int hotbarCompactExpandedOffsetX = 0; // navigating the hotbar in inventory mode will expand the centers of groups out by this much
int hotbarOffsetY = 0;
int hotbarCompactOffsetY = 0;

int xpbarOffsetWidth = 0;
int xpbarOffsetY = 0;
int xpbarCompactOffsetWidth = 0;
int xpbarCompactOffsetY = 0;

int hpmpbarOffsetWidth = 0;
int hpmpbarOffsetX = 0;
int hpmpbarOffsetY = 0;
int hpmpbarCompactOffsetWidth = 0;
int hpmpbarCompactOffsetX = 0;
int hpmpbarCompactOffsetY = 0;

int hpmpbarMaxWidthAmount = 160; // hp / mp value that makes the bar 100% size
int hpmpbarIntervalToIncreaseWidth = 5; // hp / mp grows every x max hp/mp value
int hpmpbarIntervalStartValue = 20; // hp / mp of this value is the smallest interval
int hpmpbarBasePercentSize = 30; // smallest width at low hp/mp value
real_t hpmpbarWidthIncreasePercentOnInterval = 2.5; // % to grow bar per interval
int hpmpbarCompactMaxWidthAmount = 160; // hp / mp value that makes the bar 100% size
int hpmpbarCompactIntervalToIncreaseWidth = 5; // hp / mp grows every x max hp/mp value
int hpmpbarCompactIntervalStartValue = 20; // hp / mp of this value is the smallest interval
int hpmpbarCompactBasePercentSize = 30; // smallest width at low hp/mp value
real_t hpmpbarCompactWidthIncreasePercentOnInterval = 2.5; // % to grow bar per interval

bool bUsePreciseFieldTextReflow = true;
bool bUseSelectedSlotCycleAnimation = false; // probably not gonna use, but can enable
CustomColors_t hudColors;
EnemyBarSettings_t enemyBarSettings;
StatusEffectQueue_t StatusEffectQueue[MAXPLAYERS] = { {0}, {1}, {2}, {3} };
std::unordered_map<int, StatusEffectQueue_t::EffectDefinitionEntry_t> StatusEffectQueue_t::StatusEffectDefinitions_t::allEffects;
std::unordered_map<int, StatusEffectQueue_t::EffectDefinitionEntry_t> StatusEffectQueue_t::StatusEffectDefinitions_t::allSustainedSpells;
Uint32 StatusEffectQueue_t::StatusEffectDefinitions_t::tooltipDescColor = 0xFFFFFFFF;
Uint32 StatusEffectQueue_t::StatusEffectDefinitions_t::tooltipHeadingColor = 0xFFFFFFFF;
Uint32 StatusEffectQueue_t::StatusEffectDefinitions_t::notificationTextColor = 0xFFFFFFFF;
std::string StatusEffectQueue_t::StatusEffectDefinitions_t::notificationFont = "fonts/pixelmix.ttf#16#2";

std::string formatSkillSheetEffects(int playernum, int proficiency, std::string& tag, std::string& rawValue);

int GAMEUI_FRAMEDATA_ANIMATING_ITEM = 1;
int GAMEUI_FRAMEDATA_ALCHEMY_ITEM = 2;
int GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT = 3;
int GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_ENTRY = 4;

void capitalizeString(std::string& str)
{
	if ( str.size() < 1 ) { return; }
	char letter = str[0];
	if ( letter >= 'a' && letter <= 'z' )
	{
		str[0] = toupper(letter);
	}
}

void uppercaseString(std::string& str)
{
	if ( str.size() < 1 ) { return; }
	for ( auto& letter : str )
	{
		if ( letter >= 'a' && letter <= 'z' )
		{
			letter = toupper(letter);
		}
	}
}

void camelCaseString(std::string& str)
{
	if ( str.size() < 1 ) { return; }
	char prevLetter = ' ';
	for ( auto& letter : str )
	{
		if ( letter >= 'a' && letter <= 'z' && prevLetter == ' ' )
		{
			letter = toupper(letter);
		}
		prevLetter = letter;
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
				i->pos.w = (int)imgGet->getWidth();
				i->pos.h = (int)imgGet->getHeight();
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
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Base_00.png", "hp img base");

		const int progressBarHeight = 22;
		auto fadeProgressBase = fadeFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPMidFade_00.png", "hp img fade bot");
		auto fadeProgress = fadeFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPMidFade_00.png", "hp img fade");
		auto fadeProgressEndCap = fadeFrame->addImage(SDL_Rect{
			fadeProgress->pos.x + fadeProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPEndFade_00.png", "hp img fade endcap");

		auto numbase = foregroundFrame->addImage(SDL_Rect{ 0, 4, 48, 26 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPNumBase_00.png", "hp img value");
		auto div = foregroundFrame->addImage(SDL_Rect{ 46, 0, 8, 34 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Separator_00.png", "hp img div");

		auto currentProgressBase = foregroundFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_00.png", "hp img progress bot");
		auto currentProgress = foregroundFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_00.png", "hp img progress");
		auto currentProgressEndCap = foregroundFrame->addImage(SDL_Rect{
			currentProgress->pos.x + currentProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_00.png", "hp img progress endcap");
		auto currentProgressEndCapFlash = foregroundFrame->addImage(SDL_Rect{
			currentProgress->pos.x + currentProgress->pos.w - 14, 6, 22, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F00.png", "hp img progress endcap flash");
		currentProgressEndCapFlash->disabled = true;

		const int endCapWidth = 16;
		auto endCap = foregroundFrame->addImage(SDL_Rect{ pos.w - endCapWidth, 0, endCapWidth, barTotalHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_EndCap_00.png", "hp img endcap");

		auto div25Percent = foregroundFrame->addImage(SDL_Rect{ 0, 8, 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Divider_01.png", "hp img div 25pc");
		div25Percent->disabled = true;
		auto div50Percent = foregroundFrame->addImage(SDL_Rect{ 0, 8, 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Divider_01.png", "hp img div 50pc");
		div50Percent->disabled = true;
		auto div75Percent = foregroundFrame->addImage(SDL_Rect{ 0, 8, 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Divider_01.png", "hp img div 75pc");
		div75Percent->disabled = true;

		auto font = "fonts/pixel_maz.ttf#32#2";
		auto hptext = foregroundFrame->addField("hp text", 16);
		hptext->setText("0");
		hptext->setSize(numbase->pos);
		hptext->setFont(font);
		hptext->setVJustify(Field::justify_t::CENTER);
		hptext->setHJustify(Field::justify_t::CENTER);
		hptext->setColor(makeColor( 255, 255, 255, 255));
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
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Base_00.png", "mp img base");

		const int progressBarHeight = 22;
		auto fadeProgressBase = fadeFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPMidFade_00.png", "mp img fade bot");
		auto fadeProgress = fadeFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPMidFade_00.png", "mp img fade");
		auto fadeProgressEndCap = fadeFrame->addImage(SDL_Rect{
			fadeProgress->pos.x + fadeProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPEndFade_00.png", "mp img fade endcap");

		auto numbase = foregroundFrame->addImage(SDL_Rect{ 0, 4, 48, 26 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPNumBase_00.png", "mp img value");
		auto div = foregroundFrame->addImage(SDL_Rect{ 46, 0, 8, 34 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Separator_00.png", "mp img div");

		auto currentProgressBase = foregroundFrame->addImage(SDL_Rect{ 54, 6, 6, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_00.png", "mp img progress bot");
		auto currentProgress = foregroundFrame->addImage(SDL_Rect{ 60, 6, barWidth - 60 - 8,  progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_00.png", "mp img progress");
		auto currentProgressEndCap = foregroundFrame->addImage(SDL_Rect{
			currentProgress->pos.x + currentProgress->pos.w, 6, 8, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_00.png", "mp img progress endcap");
		auto currentProgressEndCapFlash = foregroundFrame->addImage(SDL_Rect{
			currentProgress->pos.x + currentProgress->pos.w - 14, 6, 22, progressBarHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00.png", "mp img progress endcap flash");
		currentProgressEndCapFlash->disabled = true;

		const int endCapWidth = 16;
		auto endCap = foregroundFrame->addImage(SDL_Rect{ pos.w - endCapWidth, 0, endCapWidth, barTotalHeight }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_EndCap_00.png", "mp img endcap");

		auto div25Percent = foregroundFrame->addImage(SDL_Rect{ 0, 8, 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Divider_01.png", "mp img div 25pc");
		div25Percent->disabled = true;
		auto div50Percent = foregroundFrame->addImage(SDL_Rect{ 0, 8, 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Divider_01.png", "mp img div 50pc");
		div50Percent->disabled = true;
		auto div75Percent = foregroundFrame->addImage(SDL_Rect{ 0, 8, 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/HUD/hpmpbars/HUD_Bars_Divider_01.png", "mp img div 75pc");
		div75Percent->disabled = true;

		auto font = "fonts/pixel_maz.ttf#32#2";
		auto mptext = foregroundFrame->addField("mp text", 16);
		mptext->setText("0");
		mptext->setSize(numbase->pos);
		mptext->setFont(font);
		mptext->setVJustify(Field::justify_t::CENTER);
		mptext->setHJustify(Field::justify_t::CENTER);
		mptext->setColor(makeColor( 255, 255, 255, 255));
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
	const int barStartY = (hud_t.hudFrame->getSize().h - hud_t.ENEMYBAR_FRAME_START_Y - 100);
	const int barWidth = hud_t.ENEMYBAR_FRAME_WIDTH;

	SDL_Rect pos{ (hud_t.hudFrame->getSize().w / 2) - barWidth / 2 - 6, barStartY, barWidth, barTotalHeight };
	frame->setSize(pos);

	auto bg = frame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Back_Body_01.png", "base img");
	bg->pos.x = 6;
	bg->pos.h = 34;
	bg->pos.y = 4;
	bg->pos.w = 548;
	bg->color = makeColor(255, 255, 255, 255);

	auto bgEndCap = frame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Back_Cap_01.png", "base img endcap");
	bgEndCap->pos.x = bg->pos.x + bg->pos.w;
	bgEndCap->pos.h = bg->pos.h;
	bgEndCap->pos.y = bg->pos.y;
	bgEndCap->pos.w = 8;
	bgEndCap->color = bg->color;

	auto dmgFrame = frame->addFrame("bar dmg frame");
	dmgFrame->setSize(SDL_Rect{ bg->pos.x, bg->pos.y, bg->pos.w + bgEndCap->pos.w, 34 });
	dmgFrame->setInheritParentFrameOpacity(false);
	auto dmg = dmgFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_DMG_Body_01.png", "dmg img");
	dmg->pos.x = 0;
	dmg->pos.h = bg->pos.h;
	dmg->pos.y = 0;
	dmg->pos.w = 548 / 2 + 100;
	dmg->color = makeColor(255, 255, 255, 224);

	auto dmgEndCap = dmgFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_DMG_Cap_01.png", "dmg img endcap");
	dmgEndCap->pos.x = dmg->pos.w;
	dmgEndCap->pos.h = dmg->pos.h;
	dmgEndCap->pos.y = 0;
	dmgEndCap->pos.w = 10;
	dmgEndCap->color = dmg->color;

	/*auto bubbles = dmgFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Bubbles_00.png", "img bubbles");
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
	auto fg = progressFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Fill_Body_00.png", "progress img");
	fg->pos.x = 0;
	fg->pos.h = bg->pos.h - 4;
	fg->pos.y = 0;
	fg->pos.w = 548;

	auto fgEndCap = progressFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Fill_Cap_00.png", "progress img endcap");
	fgEndCap->pos.x = fg->pos.x + fg->pos.w;
	fgEndCap->pos.h = fg->pos.h;
	fgEndCap->pos.y = 0;
	if ( auto img = Image::get(fgEndCap->path.c_str()) )
	{
		fgEndCap->pos.w = img->getWidth();
	}

	auto skullFrame = frame->addFrame("skull frame");
	skullFrame->setSize(SDL_Rect{ 0, 0, 24, 44 });
	auto skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Face4_00.png", "skull 0 img");
	skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Face3_00.png", "skull 25 img");
	skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Face2_00.png", "skull 50 img");
	skull = skullFrame->addImage(skullFrame->getSize(), 0xFFFFFFFF, "*#images/ui/HUD/enemybar/HUD_EnemyHP_Face1_00.png", "skull 100 img");

	std::string font = "fonts/pixel_maz.ttf#32#2";
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

	//auto bg = hud_t.xpFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/xpbar/HUD_Bars_Base_00.png", "xp img base");
	auto bg = hud_t.xpFrame->addImage(pos, 0xFFFFFFFF, "*#images/ui/HUD/xpbar/HUD_Exp_Surround_01.png", "xp img base");
	bg->pos.x = 0;
	bg->pos.h = 26;
	bg->pos.y = 4;
	auto bgFlair = hud_t.xpFrame->addImage(SDL_Rect{4, 10, 0, 14}, makeColor(255, 255, 255, 192), "*#images/ui/HUD/xpbar/HUD_Exp_GlassShine_03.png", "xp img base flair");

	// xpProgress only adjusts width
	const int progressBarHeight = 22;
	/*auto xpProgress = hud_t.xpFrame->addImage(SDL_Rect{ 0, 6, 1, progressBarHeight }, 0xFFFFFFFF,
		"*#images/ui/HUD/xpbar/HUD_Bars_ExpMid_00.png", "xp img progress");*/
	auto progressClipFrame = hud_t.xpFrame->addFrame("xp progress clipping frame");
	progressClipFrame->setSize(SDL_Rect{ 0, 6, 1, progressBarHeight });
	auto progressClipImg = progressClipFrame->addImage(SDL_Rect{ 0, 0, 634, progressBarHeight }, 0xFFFFFFFF,
		"*#images/ui/HUD/xpbar/HUD_Exp_SandBody2_00.png", "xp img progress clipped");

	auto xpProgress = hud_t.xpFrame->addImage(SDL_Rect{ 0, 6, 1, progressBarHeight }, 0xFFFFFFFF,
		"*#images/ui/HUD/xpbar/HUD_Exp_SandBody2_00.png", "xp img progress");

	// xpProgressEndCap only adjusts x position based on xpProgress->pos.x + xpProgress->pos.w
	/*auto xpProgressEndCap = hud_t.xpFrame->addImage(SDL_Rect{0, 6, 8, progressBarHeight }, 0xFFFFFFFF,
		"*#images/ui/HUD/xpbar/HUD_Bars_ExpEnd_00.png", "xp img progress endcap");*/
	auto xpProgressEndCap = hud_t.xpFrame->addImage(SDL_Rect{ 0, 6, 38, progressBarHeight }, 0xFFFFFFFF,
		"*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00.png", "xp img progress endcap");


	const int endCapWidth = 26;
	SDL_Rect endCapPos {0, 0, endCapWidth, xpBarTotalHeight};
	auto endCapLeft = hud_t.xpFrame->addImage(endCapPos, 0xFFFFFFFF, "*#images/ui/HUD/xpbar/HUD_Bars_ExpCap1_00.png", "xp img endcap left");
	endCapLeft->ontop = true;
	endCapPos.x = pos.w - endCapPos.w;
	auto endCapRight = hud_t.xpFrame->addImage(endCapPos, 0xFFFFFFFF, "*#images/ui/HUD/xpbar/HUD_Bars_ExpCap2_00.png", "xp img endcap right");
	endCapRight->ontop = true;

	const int textWidth = 48;
	auto font = "fonts/pixel_maz.ttf#32#2";
	auto textStatic = hud_t.xpFrame->addField("xp text static", 16);
	textStatic->setText("/ 100");
	textStatic->setOntop(true);
	textStatic->setSize(SDL_Rect{ pos.w / 2 - 4, 0, textWidth, pos.h }); // x - 4 to center the slash
	textStatic->setFont(font);
	textStatic->setVJustify(Field::justify_t::CENTER);
	textStatic->setHJustify(Field::justify_t::LEFT);
	textStatic->setColor(makeColor( 255, 255, 255, 255));

	auto text = hud_t.xpFrame->addField("xp text current", 16);
	text->setText("0");
	text->setOntop(true);
	text->setSize(SDL_Rect{ pos.w / 2 - (4 * 2) - textWidth, 0, textWidth, pos.h }); // x - 4 to center the slash
	text->setFont(font);
	text->setVJustify(Field::justify_t::CENTER);
	text->setHJustify(Field::justify_t::RIGHT);
	text->setColor(makeColor( 255, 255, 255, 255));
}

void createHotbar(const int player)
{
	auto& hotbar_t = players[player]->hotbar;
	if ( !hotbar_t.hotbarFrame )
	{
		return;
	}
	Uint32 color = makeColor( 255, 255, 255, hotbarSlotOpacity);
	SDL_Rect slotPos{ 0, 0, hotbar_t.getSlotSize(), hotbar_t.getSlotSize() };
	std::array<int, NUM_HOTBAR_SLOTS> slotCreationOrder = {
		0, 2, 3, 5, 6, 8, 9, 1, 4, 7
	};
	for ( auto& i : slotCreationOrder )
	{
		char slotname[32];
		snprintf(slotname, sizeof(slotname), "hotbar slot %d", i);
		auto slot = hotbar_t.hotbarFrame->addFrame(slotname);
		slot->setSize(slotPos);
		slot->addImage(slotPos, color, "*#images/ui/HUD/hotbar/HUD_Quickbar_Slot_Box_02.png", "slot img");
		hotbar_t.hotbarSlotFrames[i] = slot;

		char glyphname[32];
		snprintf(glyphname, sizeof(glyphname), "hotbar glyph %d", i);
		auto path = Input::getGlyphPathForInput("ButtonA", false);
		auto glyph = hotbar_t.hotbarFrame->addImage(slotPos, 0xFFFFFFFF, path.c_str(), glyphname);
		glyph->disabled = true;
	}

	auto font = "fonts/pixel_maz.ttf#32#2";

	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		auto slot = hotbar_t.getHotbarSlotFrame(i);
		assert(slot);

		auto itemSlot = slot->addFrame("hotbar slot item");
		SDL_Rect itemSlotTempSize = slot->getSize();
		itemSlotTempSize.w -= 4;
		itemSlotTempSize.h -= 4;
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
	highlightFrame->addImage(slotPos, color, "*#images/ui/HUD/hotbar/HUD_Quickbar_Slot_HighlightBox_02.png", "highlight img");

	auto itemSlot = highlightFrame->addFrame("hotbar slot item");
	SDL_Rect itemSlotTempSize = slotPos;
	itemSlotTempSize.w -= 4;
	itemSlotTempSize.h -= 4;
	itemSlot->setSize(itemSlotTempSize);
	createPlayerInventorySlotFrameElements(itemSlot); // slightly shrink to align inner item elements within rect.
	itemSlot->setSize(highlightFrame->getSize());

	{
		auto oldSelectedFrame = hotbar_t.hotbarFrame->addFrame("hotbar old selected item");
		SDL_Rect oldSelectedFramePos = slotPos;
		oldSelectedFramePos.w -= 2;
		oldSelectedFramePos.h -= 2;
		oldSelectedFrame->setSize(oldSelectedFramePos);
		oldSelectedFrame->setDisabled(true);

		const int itemSpriteSize = players[oldSelectedFrame->getOwner()]->inventoryUI.getItemSpriteSize();
		SDL_Rect itemSpriteBorder{ 5, 5, itemSpriteSize, itemSpriteSize };

		color = makeColor( 0, 255, 255, 255);
		auto oldImg = oldSelectedFrame->addImage(itemSpriteBorder,
			makeColor( 255, 255, 255, 128), "", "hotbar old selected item");
		oldImg->disabled = true;
		oldSelectedFrame->addImage(SDL_Rect{ 0, 0, oldSelectedFrame->getSize().w, oldSelectedFrame->getSize().h },
			color, "*images/system/hotbar_slot.png", "hotbar old selected highlight");

		auto oldCursorFrame = hotbar_t.hotbarFrame->addFrame("hotbar old item cursor");
		oldCursorFrame->setSize(SDL_Rect{ 0, 0, oldSelectedFramePos.w + 16, oldSelectedFramePos.h + 16 });
		oldCursorFrame->setDisabled(true);
		color = makeColor( 255, 255, 255, oldSelectedCursorOpacity);
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_TL.png", "hotbar old cursor topleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_TR.png", "hotbar old cursor topright");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_BL.png", "hotbar old cursor bottomleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_BR.png", "hotbar old cursor bottomright");

		auto cursorFrame = hotbar_t.hotbarFrame->addFrame("shootmode selected item cursor");
		cursorFrame->setSize(SDL_Rect{ 0, 0, oldSelectedFramePos.w + 16, oldSelectedFramePos.h + 16 });
		cursorFrame->setDisabled(true);
		color = makeColor( 255, 255, 255, selectedCursorOpacity);
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_TL.png", "shootmode selected cursor topleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_TR.png", "shootmode selected cursor topright");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_BL.png", "shootmode selected cursor bottomleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_BR.png", "shootmode selected cursor bottomright");
	}

	auto cancelPromptTxt = hotbar_t.hotbarFrame->addField("hotbar cancel prompt", 32);
	cancelPromptTxt->setText(language[3063]);
	cancelPromptTxt->setSize(SDL_Rect{ 0, 0, 100, 24 });
	cancelPromptTxt->setDisabled(true);
	cancelPromptTxt->setFont(font);
	cancelPromptTxt->setVJustify(Field::justify_t::TOP);
	cancelPromptTxt->setHJustify(Field::justify_t::LEFT);
	auto cancelPromptGlyph = hotbar_t.hotbarFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "", "hotbar cancel glyph");
	cancelPromptGlyph->disabled = true;

	auto text = highlightFrame->addField("slot num text", 4);
	text->setText("");
	text->setSize(SDL_Rect{ 0, -4, slotPos.w, slotPos.h });
	text->setFont(font);
	text->setVJustify(Field::justify_t::TOP);
	text->setHJustify(Field::justify_t::LEFT);
	text->setOntop(true);
}

void createUINavigation(const int player)
{
	auto& hud_t = players[player]->hud;
	auto& uiNavFrame = hud_t.uiNavFrame;
	uiNavFrame = hud_t.hudFrame->addFrame("ui navigation");
	uiNavFrame->setHollow(true);
	uiNavFrame->setBorder(0);
	uiNavFrame->setOwner(player);
	uiNavFrame->setSize(SDL_Rect{ 0, 0, hud_t.hudFrame->getSize().w, hud_t.hudFrame->getSize().h });
	uiNavFrame->setDisabled(true);
	{
		const int glyphSize = 32;
		const char* buttonFont = "fonts/pixel_maz.ttf#32#2";
		auto magicButton = uiNavFrame->addButton("magic button");
		magicButton->setText(language[4115]);
		magicButton->setFont(buttonFont);
		magicButton->setBackground("*#images/ui/HUD/HUD_Button_Base_Small_00.png");
		magicButton->setSize(SDL_Rect{ 0, 0, 98, 38 });
		magicButton->setHideGlyphs(true);
		magicButton->setHideKeyboardGlyphs(true);
		magicButton->setHideSelectors(true);
		magicButton->setMenuConfirmControlType(0);
		magicButton->setColor(makeColor(255, 255, 255, 191));
		magicButton->setHighlightColor(makeColor(255, 255, 255, 255));
		magicButton->setCallback([](Button& button) {
			messagePlayer(button.getOwner(), MESSAGE_DEBUG, "%d: Magic button clicked", button.getOwner());
			if ( inputs.getVirtualMouse(button.getOwner())->draw_cursor )
			{
				// prevent 1 frame flickering of hud.cursor after click
				players[button.getOwner()]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_INVENTORY);
			}
			if ( players[button.getOwner()]->inventory_mode == INVENTORY_MODE_ITEM )
			{
				players[button.getOwner()]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
				players[button.getOwner()]->gui_mode = GUI_MODE_INVENTORY;
				players[button.getOwner()]->inventoryUI.cycleInventoryTab();
				players[button.getOwner()]->inventoryUI.spellPanel.openSpellPanel();
			}
			else if ( players[button.getOwner()]->inventory_mode == INVENTORY_MODE_SPELL )
			{
				players[button.getOwner()]->GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
				players[button.getOwner()]->gui_mode = GUI_MODE_INVENTORY;
				players[button.getOwner()]->inventoryUI.cycleInventoryTab();
				players[button.getOwner()]->inventoryUI.spellPanel.closeSpellPanel();
			}
		});
		auto magicButtonGlyph = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "magic button glyph")->disabled = true;

		auto statusButton = uiNavFrame->addButton("status button");
		statusButton->setText(language[4118]);
		statusButton->setFont(buttonFont);
		statusButton->setBackground("*#images/ui/HUD/HUD_Button_Base_Small_00.png");
		statusButton->setSize(SDL_Rect{ 0, 0, 98, 38 });
		statusButton->setHideGlyphs(true);
		statusButton->setHideKeyboardGlyphs(true);
		statusButton->setHideSelectors(true);
		statusButton->setMenuConfirmControlType(0);
		statusButton->setColor(makeColor(255, 255, 255, 191));
		statusButton->setHighlightColor(makeColor(255, 255, 255, 255));
		statusButton->setCallback([](Button& button) {
			messagePlayer(button.getOwner(), MESSAGE_DEBUG, "%d: Status button clicked", button.getOwner());
			if ( players[button.getOwner()]->hud.compactLayoutMode != Player::HUD_t::COMPACT_LAYOUT_CHARSHEET )
			{
				players[button.getOwner()]->inventoryUI.slideOutPercent = 1.0;
			}
			players[button.getOwner()]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_CHARSHEET;
			if ( inputs.getVirtualMouse(button.getOwner())->draw_cursor )
			{
				// prevent 1 frame flickering of hud.cursor after click
				players[button.getOwner()]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_INVENTORY);
			}
			players[button.getOwner()]->GUI.activateModule(Player::GUI_t::MODULE_CHARACTERSHEET);
			if ( players[button.getOwner()]->characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED )
			{
				players[button.getOwner()]->characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
			}
			players[button.getOwner()]->GUI.warpControllerToModule(false);
		});
		auto statusButtonGlyph = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "status button glyph")->disabled = true;

		auto itemsButton = uiNavFrame->addButton("items button");
		itemsButton->setText(language[4116]);
		itemsButton->setFont(buttonFont);
		itemsButton->setBackground("*#images/ui/HUD/HUD_Button_Base_Small_00.png");
		itemsButton->setSize(SDL_Rect{ 0, 0, 98, 38 });
		itemsButton->setHideGlyphs(true);
		itemsButton->setHideKeyboardGlyphs(true);
		itemsButton->setHideSelectors(true);
		itemsButton->setMenuConfirmControlType(0);
		itemsButton->setColor(makeColor(255, 255, 255, 191));
		itemsButton->setHighlightColor(makeColor(255, 255, 255, 255));
		itemsButton->setCallback([](Button& button) {
			messagePlayer(button.getOwner(), MESSAGE_DEBUG, "%d: Item button clicked", button.getOwner());
			players[button.getOwner()]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
			if ( inputs.getVirtualMouse(button.getOwner())->draw_cursor )
			{
				// prevent 1 frame flickering of hud.cursor after click
				players[button.getOwner()]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_INVENTORY);
			}
			if ( players[button.getOwner()]->inventory_mode == INVENTORY_MODE_SPELL
				&& players[button.getOwner()]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
			{
				players[button.getOwner()]->inventoryUI.cycleInventoryTab();
			}
			else
			{
				players[button.getOwner()]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM, Player::GUI_t::MODULE_INVENTORY);
			}
		});

		auto itemsButtonGlyph = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "items button glyph")->disabled = true;

		auto skillsButton = uiNavFrame->addButton("skills button");
		skillsButton->setText(language[4117]);
		skillsButton->setFont(buttonFont);
		skillsButton->setBackground("*#images/ui/HUD/HUD_Button_Base_Small_00.png");
		skillsButton->setSize(SDL_Rect{ 0, 0, 98, 38 });
		skillsButton->setHideGlyphs(true);
		skillsButton->setHideKeyboardGlyphs(true);
		skillsButton->setHideSelectors(true);
		skillsButton->setMenuConfirmControlType(0);
		skillsButton->setColor(makeColor(255, 255, 255, 191));
		skillsButton->setHighlightColor(makeColor(255, 255, 255, 255));
		skillsButton->setCallback([](Button& button) {
			messagePlayer(button.getOwner(), MESSAGE_DEBUG, "%d: Skills button clicked", button.getOwner());
			players[button.getOwner()]->skillSheet.openSkillSheet();
		});

		auto skillsButtonGlyph = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "skills button glyph")->disabled = true;
	}
	{
		const int glyphSize = 32;
		const char* navFont = "fonts/pixel_maz.ttf#32#2";
		auto leftBumperNavigationTxt = uiNavFrame->addField("left bumper txt", 64);
		leftBumperNavigationTxt->setFont(navFont);
		leftBumperNavigationTxt->setHJustify(Field::justify_t::RIGHT);
		leftBumperNavigationTxt->setVJustify(Field::justify_t::CENTER);
		leftBumperNavigationTxt->setDisabled(true);

		auto leftBumperNavigationImg = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "left bumper img");
		leftBumperNavigationImg->disabled = true;

		auto rightBumperNavigationTxt = uiNavFrame->addField("right bumper txt", 64);
		rightBumperNavigationTxt->setFont(navFont);
		rightBumperNavigationTxt->setVJustify(Field::justify_t::CENTER);
		rightBumperNavigationTxt->setHJustify(Field::justify_t::LEFT);

		auto rightBumperNavigationImg = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "right bumper img");
		rightBumperNavigationImg->disabled = true;

		auto leftTriggerNavigationTxt = uiNavFrame->addField("left trigger txt", 64);
		leftTriggerNavigationTxt->setFont(navFont);
		leftTriggerNavigationTxt->setHJustify(Field::justify_t::RIGHT);
		leftTriggerNavigationTxt->setVJustify(Field::justify_t::CENTER);
		leftTriggerNavigationTxt->setDisabled(true);

		auto leftTriggerNavigationImg = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "left trigger img");
		leftTriggerNavigationImg->disabled = true;

		auto rightTriggerNavigationTxt = uiNavFrame->addField("right trigger txt", 64);
		rightTriggerNavigationTxt->setFont(navFont);
		rightTriggerNavigationTxt->setVJustify(Field::justify_t::CENTER);
		rightTriggerNavigationTxt->setHJustify(Field::justify_t::LEFT);

		auto rightTriggerNavigationImg = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "right trigger img");
		rightTriggerNavigationImg->disabled = true;

		auto additionalNavigationTxt = uiNavFrame->addField("additional txt", 64);
		additionalNavigationTxt->setFont(navFont);
		additionalNavigationTxt->setVJustify(Field::justify_t::CENTER);
		additionalNavigationTxt->setHJustify(Field::justify_t::LEFT);

		auto additionalNavigationImg = uiNavFrame->addImage(SDL_Rect{ 0, 0, glyphSize, glyphSize },
			0xFFFFFFFF, "images/system/white.png", "additional img");
		additionalNavigationImg->disabled = true;
	}
}

void Player::HUD_t::updateUINavigation()
{
	if ( !hudFrame )
	{
		return;
	}

	if ( !uiNavFrame )
	{
		createUINavigation(player.playernum);
		if ( !uiNavFrame )
		{
			return;
		}
	}

	bool leftTriggerPressed = Input::inputs[player.playernum].consumeBinaryToggle("UINavLeftTrigger")
		&& player.bControlEnabled && !gamePaused && !player.usingCommand();
	bool rightTriggerPressed = Input::inputs[player.playernum].consumeBinaryToggle("UINavRightTrigger")
		&& player.bControlEnabled && !gamePaused && !player.usingCommand();

	bShowUINavigation = false;
	if ( player.gui_mode != GUI_MODE_NONE 
		&& player.gui_mode != GUI_MODE_FOLLOWERMENU 
		&& player.gui_mode != GUI_MODE_SIGN
		&& player.isLocalPlayer() && !player.shootmode )
	{
		/*if ( player.bUseCompactGUIWidth() * Frame::virtualScreenX || (keystatus[SDL_SCANCODE_Y] && enableDebugKeys) )
		{
			bShowUINavigation = true;
		}*/
		bShowUINavigation = true;
	}

	if ( !bShowUINavigation )
	{
		uiNavFrame->setDisabled(true);
		return;
	}
	uiNavFrame->setDisabled(false);
	uiNavFrame->setSize(SDL_Rect{ 0, 0, hudFrame->getSize().w, hudFrame->getSize().h });

	auto leftBumperModule = player.GUI.handleModuleNavigation(true, true);
	auto leftBumperTxt = uiNavFrame->findField("left bumper txt");
	leftBumperTxt->setDisabled(true);
	auto leftBumperGlyph = uiNavFrame->findImage("left bumper img");
	leftBumperGlyph->disabled = true;
	auto rightBumperModule = player.GUI.handleModuleNavigation(true, false);
	auto rightBumperTxt = uiNavFrame->findField("right bumper txt");
	rightBumperTxt->setDisabled(true);
	auto rightBumperGlyph = uiNavFrame->findImage("right bumper img");
	rightBumperGlyph->disabled = true;

	auto leftTriggerTxt = uiNavFrame->findField("left trigger txt");
	leftTriggerTxt->setDisabled(true);
	auto leftTriggerGlyph = uiNavFrame->findImage("left trigger img");
	leftTriggerGlyph->disabled = true;
	auto rightTriggerTxt = uiNavFrame->findField("right trigger txt");
	rightTriggerTxt->setDisabled(true);
	auto rightTriggerGlyph = uiNavFrame->findImage("right trigger img");
	rightTriggerGlyph->disabled = true;

	auto additionalTxt = uiNavFrame->findField("additional txt");
	additionalTxt->setDisabled(true);
	auto additionalGlyph = uiNavFrame->findImage("additional img");
	additionalGlyph->disabled = true;
	if ( inputs.hasController(player.playernum) && !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& !player.bUseCompactGUIWidth() )
	{
		if ( leftBumperModule != Player::GUI_t::MODULE_NONE )
		{
			switch ( leftBumperModule )
			{
				case Player::GUI_t::MODULE_INVENTORY:
				case Player::GUI_t::MODULE_SPELLS:
				case Player::GUI_t::MODULE_HOTBAR:
				case Player::GUI_t::MODULE_CHARACTERSHEET:
				case Player::GUI_t::MODULE_CHEST:
				case Player::GUI_t::MODULE_SHOP:
				case Player::GUI_t::MODULE_ALCHEMY:
					leftBumperTxt->setDisabled(false);
					leftBumperTxt->setText("/");
					break;
				default:
					break;
			}
		}
		if ( rightBumperModule != Player::GUI_t::MODULE_NONE )
		{
			switch ( rightBumperModule )
			{
				case Player::GUI_t::MODULE_INVENTORY:
				case Player::GUI_t::MODULE_SPELLS:
				case Player::GUI_t::MODULE_HOTBAR:
				case Player::GUI_t::MODULE_CHARACTERSHEET:
				case Player::GUI_t::MODULE_CHEST:
				case Player::GUI_t::MODULE_SHOP:
				case Player::GUI_t::MODULE_ALCHEMY:
					rightBumperTxt->setDisabled(false);
					rightBumperTxt->setText(language[4092]);
					break;
				default:
					break;
			}
		}
		if ( player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY || player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR )
		{

		}
		else if ( player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET )
		{
			auto selectedElement = player.characterSheet.selectedElement;
			if ( selectedElement >= Player::CharacterSheet_t::SHEET_STR && selectedElement <= Player::CharacterSheet_t::SHEET_WGT )
			{
				additionalTxt->setDisabled(false);
				additionalTxt->setText(language[4111]);
			}
			else if ( selectedElement == Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR )
			{
				additionalTxt->setDisabled(false);
				additionalTxt->setText(language[4112]);
			}
			else if ( selectedElement == Player::CharacterSheet_t::SHEET_SKILL_LIST
				|| selectedElement == Player::CharacterSheet_t::SHEET_OPEN_LOG 
				|| selectedElement == Player::CharacterSheet_t::SHEET_OPEN_MAP )
			{
				additionalTxt->setDisabled(false);
				// options to use specific text 'open log' etc
				//if ( selectedElement == Player::CharacterSheet_t::SHEET_SKILL_LIST )
				//{
				//	additionalTxt->setText(language[4095]);
				//}
				//else if ( selectedElement == Player::CharacterSheet_t::SHEET_OPEN_LOG )
				//{
				//	additionalTxt->setText(language[4106]);
				//}
				//else if ( selectedElement == Player::CharacterSheet_t::SHEET_OPEN_MAP )
				//{
				//	additionalTxt->setText(language[4105]);
				//}
				additionalTxt->setText(language[4107]); // activate
			}
			else if ( selectedElement == Player::CharacterSheet_t::SHEET_GOLD )
			{
				additionalTxt->setDisabled(false);
				if ( player.GUI.isDropdownActive() )
				{
					additionalTxt->setText(language[4053]);
				}
				else
				{
					additionalTxt->setText(language[4108]);
				}
			}
			else if ( selectedElement == Player::CharacterSheet_t::SHEET_TIMER )
			{
				additionalTxt->setDisabled(false);
				if ( player.characterSheet.showGameTimerAlways )
				{
					additionalTxt->setText(language[4110]);
				}
				else
				{
					additionalTxt->setText(language[4109]);
				}
			}
			if ( !additionalTxt->isDisabled() )
			{
				if ( selectedElement == Player::CharacterSheet_t::SHEET_GOLD
					&& player.GUI.isDropdownActive() )
				{
					additionalGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
				}
				else
				{
					additionalGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuConfirm");
				}
			}
		}
	}

	int lowestLeftY = 8;
	int lowestRightY = 8;

	int leftAnchorX = 0;
	int rightAnchorX = 0;
	PanelJustify_t justify = player.inventoryUI.inventoryPanelJustify;
	if ( player.inventoryUI.frame )
	{
		auto inventoryBgFrame = player.inventoryUI.frame->findFrame("inventory base");

		Frame::image_t* invBaseImg = inventoryBgFrame->findImage("inventory base img");

		if ( justify == PANEL_JUSTIFY_LEFT )
		{
			leftAnchorX = inventoryBgFrame->getSize().x + 8;
			leftAnchorX += invBaseImg->pos.w;

			rightAnchorX = player.inventoryUI.frame->getSize().w - leftAnchorX;
		}
		else
		{
			rightAnchorX = inventoryBgFrame->getSize().x - 8;
			rightAnchorX += invBaseImg->pos.x;

			leftAnchorX = player.inventoryUI.frame->getSize().w - rightAnchorX;
		}
	}

	if ( inputs.hasController(player.playernum) && !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& !player.bUseCompactGUIWidth() )
	{
		if ( (player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
			|| player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS
			|| player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
			|| player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
			|| player.GUI.activeModule == Player::GUI_t::MODULE_SHOP
			|| player.GUI.activeModule == Player::GUI_t::MODULE_CHEST) )
		{
			if ( !GenericGUI[player.playernum].tinkerGUI.bOpen && !GenericGUI[player.playernum].alchemyGUI.bOpen
				&& !GenericGUI[player.playernum].featherGUI.bOpen )
			{
				justify = PANEL_JUSTIFY_LEFT;
				leftTriggerGlyph->disabled = false;
				leftTriggerGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("UINavLeftTrigger");
				SDL_Rect textPos;
				textPos.x = (justify == PANEL_JUSTIFY_LEFT) ? leftAnchorX : rightAnchorX;
				textPos.y = 8;
				textPos.w = leftTriggerTxt->getTextObject()->getWidth();
				textPos.h = Font::get(leftTriggerTxt->getFont())->height() + 8;
				if ( justify == PANEL_JUSTIFY_LEFT )
				{
					leftTriggerTxt->setHJustify(Field::justify_t::LEFT);
				}
				else
				{
					leftTriggerTxt->setHJustify(Field::justify_t::RIGHT);
				}

				SDL_Rect imgPos;
				if ( auto imgGet = Image::get(leftTriggerGlyph->path.c_str()) )
				{
					imgPos.w = imgGet->getWidth();
					imgPos.h = imgGet->getHeight();
				}
				imgPos.x = (justify == PANEL_JUSTIFY_LEFT) ? leftAnchorX : rightAnchorX;
				imgPos.y = textPos.y - (imgPos.h - textPos.h) / 2;

				if ( justify == PANEL_JUSTIFY_LEFT )
				{
					leftTriggerGlyph->pos = imgPos;
					textPos.x = leftTriggerGlyph->pos.x + leftTriggerGlyph->pos.w + 8;
					leftTriggerTxt->setSize(textPos);
				}
				else
				{
					imgPos.x -= imgPos.w;
					leftTriggerGlyph->pos = imgPos;
					textPos.x = leftTriggerGlyph->pos.x - 8 - textPos.w;
					leftTriggerTxt->setSize(textPos);
				}
				leftTriggerTxt->setDisabled(false);
				if ( player.inventory_mode == INVENTORY_MODE_ITEM )
				{
					leftTriggerTxt->setText(language[4093]);
				}
				else if ( player.inventory_mode == INVENTORY_MODE_SPELL )
				{
					leftTriggerTxt->setText(language[4094]);
				}
			}

			if ( !player.inventoryUI.chestGUI.bOpen && !player.shopGUI.bOpen
				&& !GenericGUI[player.playernum].tinkerGUI.bOpen && !GenericGUI[player.playernum].alchemyGUI.bOpen
				&& !GenericGUI[player.playernum].featherGUI.bOpen )
			{
				justify = PANEL_JUSTIFY_RIGHT;
				rightTriggerGlyph->disabled = false;
				rightTriggerGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("UINavRightTrigger");
				SDL_Rect textPos;
				textPos.x = (justify == PANEL_JUSTIFY_LEFT) ? leftAnchorX : rightAnchorX;
				textPos.y = 8;
				textPos.w = rightTriggerTxt->getTextObject()->getWidth();
				textPos.h = Font::get(rightTriggerTxt->getFont())->height() + 8;
				if ( justify == PANEL_JUSTIFY_LEFT )
				{
					rightTriggerTxt->setHJustify(Field::justify_t::LEFT);
				}
				else
				{
					rightTriggerTxt->setHJustify(Field::justify_t::RIGHT);
				}

				SDL_Rect imgPos;
				if ( auto imgGet = Image::get(rightTriggerGlyph->path.c_str()) )
				{
					imgPos.w = imgGet->getWidth();
					imgPos.h = imgGet->getHeight();
				}
				imgPos.x = (justify == PANEL_JUSTIFY_LEFT) ? leftAnchorX : rightAnchorX;
				imgPos.y = textPos.y - (imgPos.h - textPos.h) / 2;

				if ( justify == PANEL_JUSTIFY_LEFT )
				{
					rightTriggerGlyph->pos = imgPos;
					textPos.x = rightTriggerGlyph->pos.x + rightTriggerGlyph->pos.w + 8;
					rightTriggerTxt->setSize(textPos);
				}
				else
				{
					imgPos.x -= imgPos.w;
					rightTriggerGlyph->pos = imgPos;
					textPos.x = rightTriggerGlyph->pos.x - 8 - textPos.w;
					rightTriggerTxt->setSize(textPos);
				}
				rightTriggerTxt->setDisabled(false);
				rightTriggerTxt->setText(language[4095]);

				if ( !additionalTxt->isDisabled() )
				{
					if ( justify == PANEL_JUSTIFY_LEFT )
					{
						additionalTxt->setHJustify(Field::justify_t::LEFT);
					}
					else
					{
						additionalTxt->setHJustify(Field::justify_t::RIGHT);
					}
					additionalGlyph->disabled = false;
					SDL_Rect textPos;
					textPos.w = additionalTxt->getTextObject()->getWidth();
					textPos.h = Font::get(additionalTxt->getFont())->height() + 8;
					if ( justify == PANEL_JUSTIFY_LEFT )
					{
						textPos.x = rightTriggerTxt->getSize().x;
					}
					else
					{
						textPos.x = rightTriggerTxt->getSize().x + rightTriggerTxt->getSize().w - textPos.w;
					}
					textPos.y = std::max(lowestRightY, rightTriggerTxt->getSize().y + rightTriggerTxt->getSize().h);

					SDL_Rect imgPos;
					if ( auto imgGet = Image::get(additionalGlyph->path.c_str()) )
					{
						imgPos.w = imgGet->getWidth();
						imgPos.h = imgGet->getHeight();
					}
					imgPos.x = rightTriggerGlyph->pos.x + rightTriggerGlyph->pos.w / 2 - imgPos.w / 2;
					imgPos.y = textPos.y - (imgPos.h - textPos.h) / 2;
					additionalGlyph->pos = imgPos;

					additionalTxt->setSize(textPos);
				}
			}
		}

		if ( leftTriggerPressed	&& !leftTriggerTxt->isDisabled() )
		{
			leftTriggerPressed = false;
			if ( !inputs.getUIInteraction(player.playernum)->selectedItem && !player.GUI.isDropdownActive()
			&& (inputs.hasController(player.playernum) && !player.shootmode
				&& (player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
					|| player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS
					|| player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
					|| player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
					|| player.GUI.activeModule == Player::GUI_t::MODULE_CHEST
					|| player.GUI.activeModule == Player::GUI_t::MODULE_SHOP)) )
			{
				player.gui_mode = GUI_MODE_INVENTORY;
				if ( player.shootmode )
				{
					player.openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM);
				}
				player.inventoryUI.cycleInventoryTab();
			}
		}

		if ( rightTriggerPressed && !rightTriggerTxt->isDisabled() )
		{
			rightTriggerPressed = false;
			if ( !inputs.getUIInteraction(player.playernum)->selectedItem && !player.GUI.isDropdownActive()
				&& (inputs.hasController(player.playernum) && !player.shootmode
					&& (player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY
						|| player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS
						|| player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
						|| player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET)) )
			{
				if ( !player.skillSheet.bSkillSheetOpen )
				{
					player.skillSheet.openSkillSheet();
				}
			}
		}

		if ( !leftTriggerTxt->isDisabled() )
		{
			lowestLeftY = std::max(lowestLeftY, leftTriggerTxt->getSize().y + leftTriggerTxt->getSize().h);
			lowestLeftY = std::max(lowestLeftY, leftTriggerGlyph->pos.y + leftTriggerGlyph->pos.h);
		}
		if ( !rightTriggerTxt->isDisabled() )
		{
			lowestRightY = std::max(lowestRightY, rightTriggerTxt->getSize().y + rightTriggerTxt->getSize().h);
			lowestRightY = std::max(lowestRightY, rightTriggerGlyph->pos.y + rightTriggerGlyph->pos.h);
		}

		if ( !leftBumperTxt->isDisabled() )
		{
			leftBumperGlyph->disabled = false;
			leftBumperGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("UINavLeftBumper");
			SDL_Rect textPos;
			textPos.x = leftTriggerGlyph->pos.x;
			textPos.y = lowestLeftY;
			textPos.w = leftBumperTxt->getTextObject()->getWidth();
			textPos.h = Font::get(leftBumperTxt->getFont())->height() + 8;

			SDL_Rect imgPos;
			if ( auto imgGet = Image::get(leftBumperGlyph->path.c_str()) )
			{
				imgPos.w = imgGet->getWidth();
				imgPos.h = imgGet->getHeight();
			}
			imgPos.x = textPos.x;
			imgPos.y = textPos.y - (imgPos.h - textPos.h) / 2;
			leftBumperGlyph->pos = imgPos;

			textPos.x = imgPos.x + 4 + imgPos.w;
			leftBumperTxt->setSize(textPos);
		}
		if ( !rightBumperTxt->isDisabled() )
		{
			rightBumperGlyph->disabled = false;
			rightBumperGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("UINavRightBumper");
			SDL_Rect textPos;
			textPos.x = leftBumperTxt->getSize().x + leftBumperTxt->getSize().w + 8;
			textPos.y = leftBumperTxt->getSize().y;
			textPos.w = rightBumperTxt->getTextObject()->getWidth();
			textPos.h = Font::get(rightBumperTxt->getFont())->height() + 8;

			SDL_Rect imgPos;
			if ( auto imgGet = Image::get(rightBumperGlyph->path.c_str()) )
			{
				imgPos.w = imgGet->getWidth();
				imgPos.h = imgGet->getHeight();
			}
			imgPos.x = textPos.x;
			imgPos.y = textPos.y - (imgPos.h - textPos.h) / 2;

			textPos.x += imgPos.w + 4;
			rightBumperTxt->setSize(textPos);
			rightBumperGlyph->pos = imgPos;
		}
	}

	auto magicButton = uiNavFrame->findButton("magic button");
	auto magicButtonGlyph = uiNavFrame->findImage("magic button glyph");
	auto itemsButton = uiNavFrame->findButton("items button");
	auto itemsButtonGlyph = uiNavFrame->findImage("items button glyph");
	auto statusButton = uiNavFrame->findButton("status button");
	auto statusButtonGlyph = uiNavFrame->findImage("status button glyph");
	auto skillsButton = uiNavFrame->findButton("skills button");
	auto skillsButtonGlyph = uiNavFrame->findImage("skills button glyph");

	struct ButtonsAndGlyphs {
		std::string name;
		Button* button = nullptr;
		Frame::image_t* glyph = nullptr;
		std::string inputName;
		CompactLayoutModes layoutMode;
		ButtonsAndGlyphs(std::string _name, 
			Button* _button, 
			Frame::image_t* _glyph,
			std::string _inputName, 
			CompactLayoutModes _layoutMode) :
			name(_name),
			button(_button),
			glyph(_glyph),
			inputName(_inputName),
			layoutMode(_layoutMode)
		{
		}
	};
	std::vector<ButtonsAndGlyphs> allButtonsAndGlyphs;
	allButtonsAndGlyphs.emplace_back(
		ButtonsAndGlyphs{ "magic button", magicButton, magicButtonGlyph, 
		"UINavLeftTrigger", COMPACT_LAYOUT_INVENTORY });
	allButtonsAndGlyphs.emplace_back(
		ButtonsAndGlyphs{ "status button", statusButton, statusButtonGlyph, 
		"UINavRightTrigger", COMPACT_LAYOUT_INVENTORY });
	allButtonsAndGlyphs.emplace_back(
		ButtonsAndGlyphs{ "items button", itemsButton, itemsButtonGlyph,
		"UINavLeftTrigger", COMPACT_LAYOUT_CHARSHEET });
	allButtonsAndGlyphs.emplace_back(
		ButtonsAndGlyphs{ "skills button", skillsButton, skillsButtonGlyph,
		"UINavRightTrigger", COMPACT_LAYOUT_CHARSHEET });

	int buttonWidth = 98;
	int buttonHeight = 38;
	int alignPaddingX = 2;
	int leftAlignX = uiNavFrame->getSize().w / 2 - buttonWidth - alignPaddingX;
	int rightAlignX = uiNavFrame->getSize().w / 2 + alignPaddingX;
	int topAlignY = 34;
	int bottomAlignY = topAlignY + 52;

	int numButtonsToShow = 2;

	for ( auto& buttonAndGlyph : allButtonsAndGlyphs )
	{
		auto& button = buttonAndGlyph.button;
		button->setDisabled(true);
		SDL_Rect buttonPos = button->getSize();
		auto& glyph = buttonAndGlyph.glyph;
		glyph->disabled = true;

		if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			button->setColor(makeColor(255, 255, 255, 255));
		}
		else
		{
			button->setColor(makeColor(255, 255, 255, 191));
		}

		if ( player.bUseCompactGUIWidth() )
		{
			if ( player.inventoryUI.chestGUI.bOpen )
			{
				if ( buttonAndGlyph.inputName == "UINavRightTrigger" )
				{
					button->setInvisible(true);
					continue;
				}
			}
			if ( player.shopGUI.bOpen )
			{
				if ( buttonAndGlyph.inputName == "UINavRightTrigger" 
					|| inputs.getVirtualMouse(player.playernum)->draw_cursor )
				{
					button->setInvisible(true);
					continue;
				}
			}
			if ( GenericGUI[player.playernum].isGUIOpen() || player.GUI.isDropdownActive()
				|| player.hud.mapWindow || player.hud.logWindow )
			{
				button->setDisabled(true);
				continue;
			}
		}

		if ( buttonAndGlyph.name == "magic button" || buttonAndGlyph.name == "items button" )
		{
			if ( inputs.bPlayerUsingKeyboardControl(player.playernum) 
				&& inputs.getVirtualMouse(player.playernum)->draw_cursor
				&& !player.bUseCompactGUIWidth()
				&& !GenericGUI[player.playernum].isGUIOpen()
				&& !player.hud.mapWindow
				&& !player.hud.logWindow )
			{
				if ( player.inventory_mode == INVENTORY_MODE_ITEM )
				{
					if ( buttonAndGlyph.name == "magic button" )
					{
						button->setDisabled(false);
						glyph->disabled = false;

						buttonPos.x = leftAnchorX;
						buttonPos.y = 8;
						buttonAndGlyph.inputName = "Spell List";
					}
				}
				else if ( player.inventory_mode == INVENTORY_MODE_SPELL )
				{
					if ( buttonAndGlyph.name == "items button" )
					{
						button->setDisabled(false);
						glyph->disabled = false;

						buttonPos.x = leftAnchorX;
						buttonPos.y = 8;
						buttonAndGlyph.inputName = "Spell List";
					}
				}
			}
			else if ( player.bUseCompactGUIWidth() )
			{
				buttonPos.x = leftAlignX;
				if ( numButtonsToShow < 4 )
				{
					buttonPos.y = topAlignY;
					if ( compactLayoutMode == COMPACT_LAYOUT_INVENTORY )
					{
						if ( buttonAndGlyph.name == "items button" && player.inventory_mode == INVENTORY_MODE_ITEM )
						{
							// leave disabled
							button->setDisabled(true);
							glyph->disabled = true;
						}
						else if ( buttonAndGlyph.name == "magic button" && player.inventory_mode == INVENTORY_MODE_SPELL )
						{
							// leave disabled
							button->setDisabled(true);
							glyph->disabled = true;
						}
						else
						{
							button->setDisabled(false);
							glyph->disabled = false;
						}
					}
					else if ( buttonAndGlyph.layoutMode == compactLayoutMode )
					{
						button->setDisabled(false);
						glyph->disabled = false;
					}
				}
				else
				{
					button->setDisabled(false);
					glyph->disabled = false;
					if ( buttonAndGlyph.name == "magic button" )
					{
						buttonPos.y = topAlignY;
					}
					else if ( buttonAndGlyph.name == "items button" )
					{
						buttonPos.y = bottomAlignY;
					}
				}
			}
			buttonPos.w = buttonWidth;
			buttonPos.h = buttonHeight;
			button->setSize(buttonPos);
		}
		if ( buttonAndGlyph.name == "status button" || buttonAndGlyph.name == "skills button" )
		{
			if ( inputs.bPlayerUsingKeyboardControl(player.playernum)
				&& inputs.getVirtualMouse(player.playernum)->draw_cursor
				&& !player.bUseCompactGUIWidth() )
			{
				// leave disabled
			}
			else if ( player.bUseCompactGUIWidth() )
			{
				buttonPos.x = rightAlignX;
				if ( numButtonsToShow < 4 )
				{
					buttonPos.y = topAlignY;
					if ( buttonAndGlyph.layoutMode == compactLayoutMode )
					{
						button->setDisabled(false);
						glyph->disabled = false;
					}
				}
				else
				{
					button->setDisabled(false);
					glyph->disabled = false;
					if ( buttonAndGlyph.name == "status button" )
					{
						buttonPos.y = topAlignY;
					}
					else if ( buttonAndGlyph.name == "skills button" )
					{
						buttonPos.y = bottomAlignY;
					}
				}
			}
			buttonPos.w = buttonWidth;
			buttonPos.h = buttonHeight;
			button->setSize(buttonPos);
		}

		if ( !button->isDisabled() )
		{
			glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding(buttonAndGlyph.inputName.c_str());
			glyph->ontop = true;
			if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				glyph->disabled = false;
			}
			else
			{
				glyph->disabled = false;
			}
			if ( auto imgGet = Image::get(glyph->path.c_str()) )
			{
				glyph->pos.w = (int)imgGet->getWidth();
				glyph->pos.h = (int)imgGet->getHeight();
			}

			glyph->pos.x = button->getSize().x + button->getSize().w / 2 - glyph->pos.w / 2; // center the x for the glyph
			const int glyphToImgPadY = 8;
			glyph->pos.y = button->getSize().y + button->getSize().h - glyphToImgPadY; // just below the button with some padding
		}
		button->setInvisible(button->isDisabled());
	}

	for ( auto& buttonAndGlyph : allButtonsAndGlyphs )
	{
		auto& button = buttonAndGlyph.button;
		if ( !button->isDisabled() && !button->isInvisible() )
		{
			if ( player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY)
				&& inputs.getVirtualMouse(player.playernum)->draw_cursor
				&& button->isHighlighted() )
			{
				player.GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_INVENTORY);
				SDL_Rect pos = button->getAbsoluteSize();
				// make sure to adjust absolute size to camera viewport
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				player.hud.setCursorDisabled(false);
				player.hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player.playernum)->draw_cursor);
			}

			if ( buttonAndGlyph.inputName == "UINavLeftTrigger" && leftTriggerPressed
				&& player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY) )
			{
				leftTriggerPressed = false;
				button->activate();
			}
			else if ( buttonAndGlyph.inputName == "UINavRightTrigger" && rightTriggerPressed
				&& player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_INVENTORY) )
			{
				rightTriggerPressed = false;
				button->activate();
			}
		}
	}
}

bool StatusEffectQueue_t::insertEffect(int effectID, int spellID)
{
	if ( spellID >= 0 && spellID < NUM_SPELLS )
	{
		effectID = spellID + kSpellEffectOffset;
		if ( StatusEffectDefinitions_t::sustainedSpellDefinitionExists(effectID) )
		{
			if ( StatusEffectDefinitions_t::getSustainedSpell(effectID).neverDisplay )
			{
				return false;
			}
		}
	}
	if ( (effectID >= 0 && effectID < NUMEFFECTS) 
		|| effectID == kEffectBread 
		|| effectID == kEffectBloodHunger
		|| effectID == kEffectAutomatonHunger
		|| effectID >= kSpellEffectOffset )
	{
		if ( StatusEffectDefinitions_t::effectDefinitionExists(effectID) )
		{
			if ( StatusEffectDefinitions_t::getEffect(effectID).neverDisplay )
			{
				return false;
			}
		}
		for ( auto& q : effectQueue )
		{
			if ( effectID == q.effect )
			{
				return false;
			}
		}
	}

	effectQueue.push_back(StatusEffectQueueEntry_t(effectID));
	effectQueue.back().pos.x = getBaseEffectPosX();
	effectQueue.back().pos.y = getBaseEffectPosY();
	notificationQueue.push_back(StatusEffectQueueEntry_t(effectID));
	notificationQueue.back().pos.x = getBaseEffectPosX();
	notificationQueue.back().pos.y = getBaseEffectPosY();

	// fall back if notificationTargetPosition doesn't have an effect to go to.
	notificationQueue.back().notificationTargetPosition.x = 0;
	notificationQueue.back().notificationTargetPosition.y = statusEffectFrame->getSize().h - notificationQueue.back().notificationTargetPosition.h;
	requiresAnimUpdate = true;
	return true;
}

std::string StatusEffectQueue_t::StatusEffectDefinitions_t::getEffectImgPath(StatusEffectQueue_t::EffectDefinitionEntry_t& entry, int variation)
{
	if ( entry.imgPath == "" )
	{
		if ( entry.imgPathVariations.size() > 0 && variation >= 0 )
		{
			return entry.imgPathVariations[std::min(variation, (int)entry.imgPathVariations.size() - 1)];
		}
		node_t* spellImageNode = nullptr;
		int spellID = entry.useSpellIDForImg;
		if ( variation >= 0 )
		{
			spellID = entry.useSpellIDForImgVariations[std::min(variation, (int)entry.useSpellIDForImgVariations.size() - 1)];
		}
		if ( spellID >= 0 && spellID < NUM_SPELLS )
		{
			if ( arachnophobia_filter )
			{
				if ( spellID == SPELL_SPIDER_FORM )
				{
					spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_FORM);
				}
				else if ( spellID == SPELL_SPRAY_WEB )
				{
					spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_WEB);
				}
				else
				{
					spellImageNode = list_Node(&items[SPELL_ITEM].images, spellID);
				}
			}
			else
			{
				spellImageNode = list_Node(&items[SPELL_ITEM].images, spellID);
			}
		}
		if ( spellImageNode )
		{
			string_t* string = (string_t*)spellImageNode->element;
			if ( string )
			{
				return string->data;
			}
		}
		return "images/sprites/null.png";
	}
	if ( entry.imgPath == "" )
	{
		return "images/sprites/null.png";
	}
	return entry.imgPath;
}

void StatusEffectQueueEntry_t::setAnimatePosition(int destx, int desty, int destw, int desth)
{
	animateStartX = pos.x;
	animateStartY = pos.y;
	animateStartW = pos.w;
	animateStartH = pos.h;
	animateSetpointX = destx;
	animateSetpointY = desty;
	animateSetpointW = destw;
	animateSetpointH = desth;
	animateX = 0.0;
	animateY = 0.0;
	animateW = 0.0;
	animateH = 0.0;
}

void StatusEffectQueueEntry_t::setAnimatePosition(int destx, int desty)
{
	animateStartX = pos.x;
	animateStartY = pos.y;
	animateStartW = pos.w;
	animateStartH = pos.h;
	animateSetpointX = destx;
	animateSetpointY = desty;
	animateSetpointW = 0;
	animateSetpointH = 0;
	animateX = 0.0;
	animateY = 0.0;
	animateW = 0.0;
	animateH = 0.0;
}

const real_t kStatusEffectQueueAnimSpeedMult = 4.0;

void StatusEffectQueueEntry_t::animate()
{
	const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - animateX)) / (5.0 / kStatusEffectQueueAnimSpeedMult);
	real_t setpointDiffY = fpsScale * std::max(.1, (1.0 - animateY)) / (5.0 / kStatusEffectQueueAnimSpeedMult);
	animateX += setpointDiffX;
	animateY += setpointDiffY;
	animateX = std::min(1.0, animateX);
	animateY = std::min(1.0, animateY);

	int destX = animateSetpointX - animateStartX;
	int destY = animateSetpointY - animateStartY;

	pos.x = animateStartX + destX * animateX;
	pos.y = animateStartY + destY * animateY;
}

void StatusEffectQueue_t::loadStatusEffectsJSON()
{
	if ( !PHYSFS_getRealDir("/data/status_effects.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/status_effects.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/status_effects.json");
		inputPath.append("/data/status_effects.json");

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
				StatusEffectDefinitions_t::reset();
				int defaultTooltipWidth = 200;
				if ( d.HasMember("default_tooltip_width") )
				{
					defaultTooltipWidth = d["default_tooltip_width"].GetInt();
				}
				if ( d.HasMember("colors") )
				{
					if ( d["colors"].HasMember("notification_text") )
					{
						StatusEffectDefinitions_t::notificationTextColor = makeColor(
							d["colors"]["notification_text"]["r"].GetInt(),
							d["colors"]["notification_text"]["g"].GetInt(),
							d["colors"]["notification_text"]["b"].GetInt(),
							d["colors"]["notification_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("tooltip_desc_text") )
					{
						StatusEffectDefinitions_t::tooltipDescColor = makeColor(
							d["colors"]["tooltip_desc_text"]["r"].GetInt(),
							d["colors"]["tooltip_desc_text"]["g"].GetInt(),
							d["colors"]["tooltip_desc_text"]["b"].GetInt(),
							d["colors"]["tooltip_desc_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("tooltip_heading_text") )
					{
						StatusEffectDefinitions_t::tooltipHeadingColor = makeColor(
							d["colors"]["tooltip_heading_text"]["r"].GetInt(),
							d["colors"]["tooltip_heading_text"]["g"].GetInt(),
							d["colors"]["tooltip_heading_text"]["b"].GetInt(),
							d["colors"]["tooltip_heading_text"]["a"].GetInt());
					}
				}
				if ( d.HasMember("notification_font") )
				{
					StatusEffectDefinitions_t::notificationFont = d["notification_font"].GetString();
				}
				if ( d.HasMember("sustained_effects") )
				{
					for ( rapidjson::Value::ConstMemberIterator itr = d["sustained_effects"].MemberBegin();
						itr != d["sustained_effects"].MemberEnd(); ++itr )
					{
						int id = -1;
						if ( itr->value.HasMember("id") )
						{
							id = itr->value["id"].GetInt();
						}
						int spellID = -1;
						if ( itr->value.HasMember("spell_id") )
						{
							spellID = itr->value["spell_id"].GetInt();
						}
						StatusEffectDefinitions_t::allSustainedSpells.insert(
							std::make_pair(spellID, EffectDefinitionEntry_t()));
						auto& entry = StatusEffectDefinitions_t::allSustainedSpells[spellID];
						entry.effect_id = id;
						entry.spell_id = spellID;
						entry.internal_name = itr->name.GetString();
						if ( itr->value["name"].IsArray() )
						{
							for ( auto arr = itr->value["name"].Begin();
								arr != itr->value["name"].End(); ++arr )
							{
								entry.nameVariations.push_back(arr->GetString());
							}
						}
						else
						{
							entry.name = itr->value["name"].GetString();
						}
						std::string buf = itr->value["desc"].GetString();
						entry.desc = "\x1E ";
						int index = 0;
						for ( auto s : buf )
						{
							if ( index == 0 && (buf[0] == '+' || buf[0] == '-') )
							{
								entry.desc = "";
							}
							entry.desc += s;
							if ( s == '\n' )
							{
								if ( index + 1 < buf.size() )
								{
									if ( buf[index + 1] == '+' || buf[index + 1] == '-' )
									{
										// skip adding dot
										++index;
										continue;
									}
								}
								entry.desc += "\x1E ";
							}
							++index;
						}
						entry.imgPath = itr->value["img_path"].GetString();
						entry.useSpellIDForImg = itr->value["img_from_spell_id"].GetInt();
						entry.neverDisplay = false;
						if ( itr->value.HasMember("never_display") )
						{
							entry.neverDisplay = itr->value["never_display"].GetBool();
						}
						entry.tooltipWidth = defaultTooltipWidth;
						if ( itr->value.HasMember("tooltip_width") )
						{
							entry.tooltipWidth = itr->value["tooltip_width"].GetInt();
						}
					}
				}
				if ( d.HasMember("effects") )
				{
					for ( rapidjson::Value::ConstMemberIterator itr = d["effects"].MemberBegin();
						itr != d["effects"].MemberEnd(); ++itr )
					{
						int id = -1;
						if ( itr->value.HasMember("id") )
						{
							id = itr->value["id"].GetInt();
						}
						StatusEffectDefinitions_t::allEffects.insert(
							std::make_pair(id, EffectDefinitionEntry_t()));
						auto& entry = StatusEffectDefinitions_t::allEffects[id];
						entry.effect_id = id;
						if ( itr->value["name"].IsArray() )
						{
							for ( auto arr = itr->value["name"].Begin();
								arr != itr->value["name"].End(); ++arr )
							{
								entry.nameVariations.push_back(arr->GetString());
							}
						}
						else
						{
							entry.name = itr->value["name"].GetString();
						}
						if ( itr->value["desc"].IsArray() )
						{
							for ( auto arr = itr->value["desc"].Begin();
								arr != itr->value["desc"].End(); ++arr )
							{
								std::string buf = arr->GetString();
								int index = 0;
								std::string formattedStr = "\x1E ";
								for ( auto s : buf )
								{
									if ( index == 0 && (buf[0] == '+' || buf[0] == '-') )
									{
										formattedStr = "";
									}
									formattedStr += s;
									if ( s == '\n' )
									{
										if ( index + 1 < buf.size() )
										{
											if ( buf[index + 1] == '+' || buf[index + 1] == '-' )
											{
												// skip adding dot
												++index;
												continue;
											}
										}
										formattedStr += "\x1E ";
									}
									++index;
								}
								entry.descVariations.push_back(formattedStr);
							}
						}
						else
						{
							std::string buf = itr->value["desc"].GetString();
							entry.desc = "\x1E ";
							int index = 0;
							for ( auto s : buf )
							{
								if ( index == 0 && (buf[0] == '+' || buf[0] == '-') )
								{
									entry.desc = "";
								}
								entry.desc += s;
								if ( s == '\n' )
								{
									if ( index + 1 < buf.size() )
									{
										if ( buf[index + 1] == '+' || buf[index + 1] == '-' )
										{
											// skip adding dot
											++index;
											continue;
										}
									}
									entry.desc += "\x1E ";
								}
								++index;
							}
						}
						entry.internal_name = itr->name.GetString();
						if ( itr->value["img_path"].IsArray() )
						{
							for ( auto arr = itr->value["img_path"].Begin();
								arr != itr->value["img_path"].End(); ++arr )
							{
								entry.imgPathVariations.push_back(arr->GetString());
							}
						}
						else
						{
							entry.imgPath = itr->value["img_path"].GetString();
						}
						if ( itr->value["img_from_spell_id"].IsArray() )
						{
							for ( auto arr = itr->value["img_from_spell_id"].Begin();
								arr != itr->value["img_from_spell_id"].End(); ++arr )
							{
								entry.useSpellIDForImgVariations.push_back(arr->GetInt());
							}
						}
						else
						{
							entry.useSpellIDForImg = itr->value["img_from_spell_id"].GetInt();
						}
						entry.sustainedSpellID = -1;
						if ( itr->value.HasMember("use_entry_for_sustained_spell") )
						{
							entry.sustainedSpellID = itr->value["use_entry_for_sustained_spell"].GetInt();
						}
						entry.neverDisplay = false;
						if ( itr->value.HasMember("never_display") )
						{
							entry.neverDisplay = itr->value["never_display"].GetBool();
						}
						entry.tooltipWidth = defaultTooltipWidth;
						if ( itr->value.HasMember("tooltip_width") )
						{
							entry.tooltipWidth = itr->value["tooltip_width"].GetInt();
						}
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
}

int StatusEffectQueue_t::getBaseEffectPosX()
{
	if ( players[player]->bUseCompactGUIHeight() )
	{
		return statusEffectFrame->getSize().w / 2 - 100;
	}
	return statusEffectFrame->getSize().w / 2 - 100;
}
int StatusEffectQueue_t::getBaseEffectPosY()
{
	if ( players[player]->bUseCompactGUIHeight() )
	{
		return statusEffectFrame->getSize().h / 2;
	}
	return statusEffectFrame->getSize().h / 2 - 50;
}

int StatusEffectQueueEntry_t::getEffectSpriteNormalWidth()
{
	if ( effect == StatusEffectQueue_t::kEffectBread
		|| effect == StatusEffectQueue_t::kEffectBloodHunger )
	{
		return 76;
	}
	else if ( effect == StatusEffectQueue_t::kEffectAutomatonHunger )
	{
		return 64;
	}
	return 32;
}
int StatusEffectQueueEntry_t::getEffectSpriteNormalHeight()
{
	if ( effect == StatusEffectQueue_t::kEffectBread
		|| effect == StatusEffectQueue_t::kEffectBloodHunger )
	{
		return 60;
	}
	else if ( effect == StatusEffectQueue_t::kEffectAutomatonHunger )
	{
		return 64;
	}
	return 32;
}

int getStatusEffectMovementAmount(int player)
{
	int movementAmount = 50;
	if ( players[player]->bUseCompactGUIHeight() )
	{
		movementAmount = 25;
	}
	return movementAmount;
}

real_t StatusEffectQueueEntry_t::getStatusEffectLargestScaling(int player)
{
	if ( effect == StatusEffectQueue_t::kEffectBread 
		|| effect == StatusEffectQueue_t::kEffectBloodHunger 
		|| effect == StatusEffectQueue_t::kEffectAutomatonHunger )
	{
		return 2.0;
	}
	return 3.0;
}

real_t StatusEffectQueueEntry_t::getStatusEffectMidScaling(int player)
{
	if ( effect == StatusEffectQueue_t::kEffectBread 
		|| effect == StatusEffectQueue_t::kEffectBloodHunger
		|| effect == StatusEffectQueue_t::kEffectAutomatonHunger )
	{
		return 1.5;
	}
	return 2.0;
}

void StatusEffectQueueEntry_t::animateNotification(int player)
{
	auto& statusEffectQueue = StatusEffectQueue[player];
	real_t animspeed = 5.0 / kStatusEffectQueueAnimSpeedMult;
	const int movementAmount = getStatusEffectMovementAmount(player);
	switch ( notificationState )
	{
		case STATE_1:
			if ( notificationStateInit == STATE_1 )
			{
				notificationStateInit = STATE_2;
				setAnimatePosition(
					statusEffectQueue.getBaseEffectPosX() - movementAmount,
					statusEffectQueue.getBaseEffectPosY() - movementAmount,
					getEffectSpriteNormalWidth(), getEffectSpriteNormalHeight());
			}
			if ( animateX >= 1.0 )
			{
				notificationState = STATE_2;
			}
			animspeed *= 2.0;
			break;
		case STATE_2:
			if ( notificationStateInit == STATE_2 )
			{
				notificationStateInit = STATE_3;
				setAnimatePosition(
					statusEffectQueue.getBaseEffectPosX() - movementAmount - (getStatusEffectLargestScaling(player) - 1.0) * getEffectSpriteNormalWidth() / 2,
					statusEffectQueue.getBaseEffectPosY() - movementAmount - (getStatusEffectLargestScaling(player) - 1.0) * getEffectSpriteNormalHeight() / 2,
					getEffectSpriteNormalWidth() * getStatusEffectLargestScaling(player),
					getEffectSpriteNormalHeight() * getStatusEffectLargestScaling(player));
			}
			if ( animateX >= 1.0 )
			{
				notificationState = STATE_3;
			}
			animspeed *= 4.0;
			break;
		case STATE_3:
			if ( notificationStateInit == STATE_3 )
			{
				notificationStateInit = STATE_4;
				setAnimatePosition(
					statusEffectQueue.getBaseEffectPosX() - movementAmount - (getStatusEffectMidScaling(player) - 1.0) * getEffectSpriteNormalWidth() / 2,
					statusEffectQueue.getBaseEffectPosY() - movementAmount - (getStatusEffectMidScaling(player) - 1.0) * getEffectSpriteNormalHeight() / 2,
					getEffectSpriteNormalWidth() * getStatusEffectMidScaling(player),
					getEffectSpriteNormalHeight() * getStatusEffectMidScaling(player));
			}
			if ( animateX >= 1.0 )
			{
				notificationState = STATE_4;
			}
			animspeed *= 4.0;
			break;
		case STATE_4:
			if ( notificationStateInit == STATE_4 )
			{
				notificationStateInit = STATE_END;
				setAnimatePosition(notificationTargetPosition.x,
					notificationTargetPosition.y,
					notificationTargetPosition.w,
					notificationTargetPosition.h);
			}
			if ( notificationTargetPosition.x != animateSetpointX
				|| notificationTargetPosition.y != animateSetpointY
				|| notificationTargetPosition.w != animateSetpointW
				|| notificationTargetPosition.h != animateSetpointH )
			{
				// re update this as our target moved.
				setAnimatePosition(notificationTargetPosition.x,
					notificationTargetPosition.y,
					notificationTargetPosition.w,
					notificationTargetPosition.h);
			}
			if ( animateX >= 1.0 )
			{
				notificationState = STATE_END;
			}
			animspeed *= 2.0;
			break;
		case STATE_END:
			return;
		default:
			break;
	}

	const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
	real_t setpointDiffX = fpsScale * std::max(.1, (1.0 - animateX)) / (animspeed);
	real_t setpointDiffY = fpsScale * std::max(.1, (1.0 - animateY)) / (animspeed);
	real_t setpointDiffW = fpsScale * std::max(.1, (1.0 - animateW)) / (animspeed);
	real_t setpointDiffH = fpsScale * std::max(.1, (1.0 - animateH)) / (animspeed);
	animateX += setpointDiffX;
	animateY += setpointDiffY;
	animateX = std::min(1.0, animateX);
	animateY = std::min(1.0, animateY);
	animateW += setpointDiffW;
	animateH += setpointDiffH;
	animateW = std::min(1.0, animateW);
	animateH = std::min(1.0, animateH);

	int destX = animateSetpointX - animateStartX;
	int destY = animateSetpointY - animateStartY;
	int destW = animateSetpointW - animateStartW;
	int destH = animateSetpointH - animateStartH;

	pos.x = animateStartX + destX * animateX;
	pos.y = animateStartY + destY * animateY;
	pos.w = animateStartW + destW * animateW;
	pos.h = animateStartH + destH * animateH;
}

void createStatusEffectQueue(const int player)
{
	auto& statusEffectQueue = StatusEffectQueue[player];
	if ( statusEffectQueue.statusEffectFrame )
	{
		return;
	}
	auto& hud_t = players[player]->hud;
	statusEffectQueue.statusEffectFrame = hud_t.hudFrame->addFrame("status effects");
	statusEffectQueue.statusEffectFrame->setHollow(true);
	statusEffectQueue.statusEffectFrame->setBorder(0);
	statusEffectQueue.statusEffectFrame->setOwner(player);
	statusEffectQueue.statusEffectFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

	auto automatonHungerFrame = statusEffectQueue.statusEffectFrame->addFrame("automaton hunger notification");
	automatonHungerFrame->setHollow(true);
	automatonHungerFrame->setDisabled(true);
	automatonHungerFrame->setSize(SDL_Rect{ 0, 0, 64, 64 });
	auto automaton_flame_img = automatonHungerFrame->addImage(SDL_Rect{ 0, 0, 64, 64 }, 0xFFFFFFFF, "images/system/Hunger_boiler_fire.png", "flame");

	auto notif_frame = statusEffectQueue.statusEffectFrame->addFrame("notification frame");
	notif_frame->setHollow(true);
	notif_frame->setDisabled(true);
	notif_frame->setSize(SDL_Rect{ 0, 0, 0, 0 });
	auto notif = notif_frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "notification img");
	notif->disabled = true;
	auto notif_txt = notif_frame->addField("notification txt", 128);
	//notif_txt->setFont("fonts/pixel_maz_multiline.ttf#16#2");
	notif_txt->setFont("fonts/pixelmix.ttf#16#2");
	notif_txt->setText("");
	notif_txt->setDisabled(true);
	notif_txt->setColor(makeColor(255, 255, 255, 255));
	notif_txt->setVJustify(Field::justify_t::CENTER);
	notif_txt->setHJustify(Field::justify_t::CENTER);

	auto innerFrame = statusEffectQueue.statusEffectFrame->addFrame("effects");
	innerFrame->setHollow(true);
}

const int breadStatusEffectHeight = 60;

std::string& StatusEffectQueue_t::EffectDefinitionEntry_t::getName(int variation)
{
	if ( variation >= 0 )
	{
		return nameVariations[std::min(variation, (int)nameVariations.size() - 1)];
	}
	return name;
}

std::string& StatusEffectQueue_t::EffectDefinitionEntry_t::getDesc(int variation)
{
	if ( variation >= 0 )
	{
		return descVariations[std::min(variation, (int)nameVariations.size() - 1)];
	}
	return desc;
}

void StatusEffectQueue_t::createStatusEffectTooltip()
{
	auto& tooltipFrame = statusEffectTooltipFrame;
	if ( tooltipFrame )
	{
		return;
	}
	char name[32];
	snprintf(name, sizeof(name), "player statusfx tooltip %d", player);
	tooltipFrame = gameUIFrame[player]->addFrame(name);
	tooltipFrame->setHollow(true);
	tooltipFrame->setDisabled(true);
	tooltipFrame->setInheritParentFrameOpacity(false);
	tooltipFrame->setBorder(0);
	tooltipFrame->setOwner(player);
	tooltipFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

	{
		Uint32 color = makeColor(255, 255, 255, 255);
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png", skillsheetEffectBackgroundImages[TOP].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_L_00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_R_00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			makeColor(22, 24, 29, 255), "images/system/white.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_BL_00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_BR_00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_B_00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
		imageSetWidthHeight9x9(tooltipFrame, skillsheetEffectBackgroundImages);

		auto heading_txt = tooltipFrame->addField("heading txt", 128);
		heading_txt->setFont("fonts/pixel_maz_multiline.ttf#16#2");
		heading_txt->setText("");
		heading_txt->setColor(makeColor(255, 255, 255, 255));
		heading_txt->setVJustify(Field::justify_t::CENTER);
		heading_txt->setHJustify(Field::justify_t::LEFT);

		auto desc_txt = tooltipFrame->addField("desc txt", 1024);
		desc_txt->setFont("fonts/pixel_maz_multiline.ttf#16#2");
		desc_txt->setText("");
		desc_txt->setColor(makeColor(0, 192, 255, 255));
		desc_txt->setVJustify(Field::justify_t::LEFT);
		desc_txt->setHJustify(Field::justify_t::LEFT);
	}
}

void Player::HUD_t::updateStatusEffectTooltip()
{
	StatusEffectQueue[player.playernum].createStatusEffectTooltip();
}

void StatusEffectQueue_t::animateStatusEffectTooltip(bool showTooltip)
{
	if ( !statusEffectTooltipFrame )
	{
		return;
	}
	auto tooltipFrame = statusEffectTooltipFrame;
	if ( static_cast<int>(tooltipFrame->getOpacity()) != tooltipOpacitySetpoint )
	{
		const real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		if ( tooltipOpacitySetpoint == 0 )
		{
			if ( ticks - tooltipDeselectedTick > 5 )
			{
				real_t factor = 10.0;
				real_t setpointDiff = fpsScale * std::max(.05, (tooltipOpacityAnimate)) / (factor);
				tooltipOpacityAnimate -= setpointDiff;
				tooltipOpacityAnimate = std::max(0.0, tooltipOpacityAnimate);
			}
		}
		else
		{
			real_t setpointDiff = fpsScale * std::max(.05, (1.0 - tooltipOpacityAnimate)) / (1);
			tooltipOpacityAnimate += setpointDiff;
			tooltipOpacityAnimate = std::min(1.0, tooltipOpacityAnimate);
		}
		tooltipFrame->setOpacity(tooltipOpacityAnimate * 100);
	}
	else
	{
		tooltipFrame->setOpacity(tooltipOpacitySetpoint);
	}

	if ( players[player]->hud.hudFrame && players[player]->hud.hudFrame->isDisabled() )
	{
		tooltipFrame->setDisabled(true);
	}

	if ( tooltipFrame->isDisabled() || !showTooltip )
	{
		tooltipShowingEffectID = -1;
		tooltipShowingEffectVariable = -1;
		return;
	}

	tooltipOpacitySetpoint = 0;
	tooltipOpacityAnimate = 1.0;
	tooltipFrame->setDisabled(false);
	tooltipFrame->setOpacity(100.0);
	tooltipDeselectedTick = ticks;
}

bool StatusEffectQueue_t::doStatusEffectTooltip(StatusEffectQueueEntry_t& entry, SDL_Rect pos)
{
	auto tooltipFrame = statusEffectTooltipFrame;
	if ( !tooltipFrame )
	{
		return false;
	}
	auto tooltipHeader = tooltipFrame->findField("heading txt");
	tooltipHeader->setColor(StatusEffectDefinitions_t::tooltipHeadingColor);
	auto tooltipDesc = tooltipFrame->findField("desc txt");
	tooltipDesc->setColor(StatusEffectDefinitions_t::tooltipDescColor);
	int fontHeight = Font::get(tooltipDesc->getFont())->height(true);
	int tooltipInnerWidth = 200;

	bool refreshTooltip = (tooltipShowingEffectID != entry.effect) || (tooltipShowingEffectVariable != entry.customVariable);
	if ( refreshTooltip )
	{
		if ( entry.effect >= StatusEffectQueue_t::kSpellEffectOffset )
		{
			int effectID = entry.effect - StatusEffectQueue_t::kSpellEffectOffset;
			if ( StatusEffectQueue_t::StatusEffectDefinitions_t::sustainedSpellDefinitionExists(effectID) )
			{
				auto& definition = StatusEffectQueue_t::StatusEffectDefinitions_t::getSustainedSpell(effectID);
				if ( effectID == SPELL_SHADOW_TAG )
				{
					int variation = 2;
					if ( players[player] && players[player]->entity )
					{
						if ( players[player]->entity->creatureShadowTaggedThisUid != 0
							&& uidToEntity(players[player]->entity->creatureShadowTaggedThisUid) )
						{
							variation = 1;
							std::string formatString = definition.getName(variation).c_str();
							char buf[256] = "";
							Entity* tagged = uidToEntity(players[player]->entity->creatureShadowTaggedThisUid);
							if ( tagged->behavior == &actMonster )
							{
								int type = tagged->getMonsterTypeFromSprite();
								if ( type != NOTHING )
								{
									snprintf(buf, 1023, formatString.c_str(), getMonsterLocalizedName((Monster)type).c_str());
								}
								else
								{
									strcpy(buf, "");
								}
							}
							else if ( tagged->behavior == &actPlayer )
							{
								snprintf(buf, 1023, formatString.c_str(), stats[tagged->skill[2]]->name);
							}
							std::string formattedName = buf;
							uppercaseString(formattedName);
							tooltipHeader->setText(formattedName.c_str());
						}
					}
					if ( variation == 2 )
					{
						std::string newHeader = definition.getName(variation).c_str();
						uppercaseString(newHeader);
						tooltipHeader->setText(newHeader.c_str());
					}
					tooltipDesc->setText(definition.getDesc(-1).c_str()); // always -1 default desc
					tooltipInnerWidth = definition.tooltipWidth;
				}
				else
				{
					int variation = -1;
					std::string newHeader = definition.getName(variation).c_str();
					uppercaseString(newHeader);
					tooltipHeader->setText(newHeader.c_str());
					tooltipDesc->setText(definition.getDesc(variation).c_str());
					tooltipInnerWidth = definition.tooltipWidth;
				}
			}
		}
		else
		{
			int effectID = entry.effect;
			if ( StatusEffectQueue_t::StatusEffectDefinitions_t::effectDefinitionExists(effectID) )
			{
				auto& definition = StatusEffectQueue_t::StatusEffectDefinitions_t::getEffect(effectID);
				int variation = -1;
				if ( effectID == EFF_SHAPESHIFT )
				{
					if ( players[player] && players[player]->entity )
					{
						switch ( players[player]->entity->effectShapeshift )
						{
							case RAT:
								variation = 0;
								break;
							case SPIDER:
								variation = 1;
								break;
							case TROLL:
								variation = 2;
								break;
							case CREATURE_IMP:
								variation = 3;
								break;
							default:
								break;
						}
					}
				}
				else if ( effectID == EFF_VAMPIRICAURA )
				{
					bool sustained = false;
					for ( node_t* node = channeledSpells[player].first; node != nullptr; node = node->next )
					{
						spell_t* spell = (spell_t*)node->element;
						if ( spell && spell->ID == SPELL_VAMPIRIC_AURA )
						{
							sustained = true;
							break;
						}
					}
					if ( sustained )
					{
						variation = 1;
					}
					else
					{
						variation = 0;
					}
				}
				else if ( effectID == StatusEffectQueue_t::kEffectBread
					|| effectID == StatusEffectQueue_t::kEffectBloodHunger )
				{
					if ( entry.customVariable >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_OVERSATIATED) )
					{
						variation = 0;
					}
					else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_STARVING) )
					{
						variation = 3;
					}
					else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_WEAK) )
					{
						variation = 2;
					}
					else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_HUNGRY) )
					{
						variation = 1;
					}
				}
				else if ( effectID == StatusEffectQueue_t::kEffectAutomatonHunger )
				{
					int nameVariation = 1;
					int descVariation = 1;
					if ( entry.customVariable >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_SUPERHEATED) )
					{
						nameVariation = 3;
						descVariation = 0;
					}
					else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
					{
						nameVariation = 5;
						if ( svFlags & SV_FLAG_HUNGER )
						{
							descVariation = 2;
						}
						else
						{
							descVariation = 3;
						}
					}
					else
					{
						nameVariation = 4;
						descVariation = 1;
					}

					std::string newHeader = definition.getName(nameVariation).c_str();
					uppercaseString(newHeader);
					tooltipHeader->setText(newHeader.c_str());
					tooltipDesc->setText(definition.getDesc(descVariation).c_str());
					tooltipInnerWidth = definition.tooltipWidth;
				}

				if ( effectID != StatusEffectQueue_t::kEffectAutomatonHunger )
				{
					std::string newHeader = definition.getName(variation).c_str();
					uppercaseString(newHeader);
					tooltipHeader->setText(newHeader.c_str());
					tooltipDesc->setText(definition.getDesc(variation).c_str());
					tooltipInnerWidth = definition.tooltipWidth;
				}
			}
		}
	}
	else
	{
		if ( entry.effect >= StatusEffectQueue_t::kSpellEffectOffset )
		{
			int effectID = entry.effect - StatusEffectQueue_t::kSpellEffectOffset;
			if ( StatusEffectQueue_t::StatusEffectDefinitions_t::sustainedSpellDefinitionExists(effectID) )
			{
				auto& definition = StatusEffectQueue_t::StatusEffectDefinitions_t::getSustainedSpell(effectID);
				tooltipInnerWidth = definition.tooltipWidth;
			}
		}
		else
		{
			int effectID = entry.effect;
			if ( StatusEffectQueue_t::StatusEffectDefinitions_t::effectDefinitionExists(effectID) )
			{
				auto& definition = StatusEffectQueue_t::StatusEffectDefinitions_t::getEffect(effectID);
				tooltipInnerWidth = definition.tooltipWidth;
			}
		}
	}

	tooltipInnerWidth = std::max(tooltipInnerWidth, (int)tooltipHeader->getTextObject()->getWidth() + (tooltipHeader->getSize().x * 2));

	const int padx = 16;
	const int pady1 = 4;
	tooltipHeader->setSize(SDL_Rect{ padx, pady1, tooltipInnerWidth, fontHeight + 4 });

	auto descPos = tooltipDesc->getSize();
	descPos.w = tooltipInnerWidth;
	tooltipDesc->setSize(descPos);
	if ( refreshTooltip )
	{
		tooltipDesc->reflowTextToFit(0);
	}

	const int pady2 = 6;
	descPos.x = padx + 4;
	descPos.y = tooltipHeader->getSize().y + tooltipHeader->getSize().h + pady2;
	descPos.h = tooltipDesc->getNumTextLines() * fontHeight + 4;
	tooltipDesc->setSize(descPos);

	tooltipFrame->setDisabled(false);
	SDL_Rect tooltipPos;
	tooltipPos.w = tooltipInnerWidth + padx * 2;
	tooltipPos.h = tooltipHeader->getSize().y + tooltipHeader->getSize().h + pady2 + descPos.h + 8;
	tooltipPos.x = pos.x + pos.w / 2;
	tooltipPos.y = pos.y - tooltipPos.h - 8;
	tooltipFrame->setSize(tooltipPos);
	imageResizeToContainer9x9(tooltipFrame, SDL_Rect{0, 0, tooltipPos.w, tooltipPos.h}, skillsheetEffectBackgroundImages);
	tooltipShowingEffectID = entry.effect;
	tooltipShowingEffectVariable = entry.customVariable;
	return true;
}

const int StatusEffectQueue_t::kEffectBread = -2;
const int StatusEffectQueue_t::kEffectBloodHunger = -3;
const int StatusEffectQueue_t::kEffectAutomatonHunger = -4;
const int StatusEffectQueue_t::kEffectBurning = -5;
const int StatusEffectQueue_t::kSpellEffectOffset = 10000;

void StatusEffectQueue_t::updateAllQueuedEffects()
{
	std::unordered_set<int> effectSet;
	for ( auto it = effectQueue.rbegin(); it != effectQueue.rend(); ++it )
	{
		effectSet.insert((*it).effect);
	}

	std::unordered_set<int> spellsActive;
	int count = 0; //This is just for debugging purposes.
	for ( node_t* node = channeledSpells[player].first; node; node = node->next, count++ )
	{
		spell_t* spell = (spell_t*)node->element;
		if ( !spell )
		{
			break;
		}
		spellsActive.insert(spell->ID);
		if ( StatusEffectDefinitions_t::sustainedSpellDefinitionExists(spell->ID) )
		{
			if ( StatusEffectDefinitions_t::getSustainedSpell(spell->ID).effect_id == -1 )
			{
				// unique sustained effect
				int effectID = spell->ID + kSpellEffectOffset;
				if ( effectSet.find(effectID) == effectSet.end() )
				{
					insertEffect(-1, spell->ID);
				}
			}
		}
	}
	for ( auto& eff : effectSet )
	{
		if ( eff >= kSpellEffectOffset && (eff - kSpellEffectOffset) == SPELL_SHADOW_TAG )
		{
			if ( players[player] && players[player]->entity
				&& players[player]->entity->creatureShadowTaggedThisUid != 0
				&& uidToEntity(players[player]->entity->creatureShadowTaggedThisUid) )
			{
				// shadow tag still active, check uid
				for ( auto it = effectQueue.rbegin(); it != effectQueue.rend(); ++it )
				{
					if ( (*it).effect == kSpellEffectOffset + SPELL_SHADOW_TAG )
					{
						if ( (*it).customVariable != players[player]->entity->creatureShadowTaggedThisUid )
						{
							deleteEffect(eff);
							break;
						}
					}
				}
			}
			else
			{
				deleteEffect(eff);
			}
		}
		else if ( eff >= kSpellEffectOffset && spellsActive.find(eff - kSpellEffectOffset) == spellsActive.end() )
		{
			// effect has expired.
			deleteEffect(eff);
		}
	}

	for ( int i = 0; i <= NUMEFFECTS; ++i )
	{
		if ( i == NUMEFFECTS )
		{
			if ( players[player] && players[player]->entity )
			{
				if ( players[player]->entity->creatureShadowTaggedThisUid != 0
					&& uidToEntity(players[player]->entity->creatureShadowTaggedThisUid) )
				{
					if ( insertEffect(-1, SPELL_SHADOW_TAG) )
					{
						effectQueue.back().customVariable = players[player]->entity->creatureShadowTaggedThisUid;
						notificationQueue.back().customVariable = players[player]->entity->creatureShadowTaggedThisUid;
					}
				}
			}
		}
	    else
	    {
		    if ( stats[player]->EFFECTS[i] )
		    {
			    if ( effectSet.find(i) == effectSet.end() )
			    {
				    insertEffect(i, -1);
			    }
		    }
		    else
		    {
			    if ( effectSet.find(i) != effectSet.end() )
			    {
				    deleteEffect(i);
			    }
		    }
		}
	}

	bool burning = false;
	if ( players[player] && players[player]->entity && players[player]->entity->flags[BURNING] )
	{
		burning = true;
		if ( effectSet.find(kEffectBurning) == effectSet.end() )
		{
			insertEffect(kEffectBurning, -1);
		}
	}
	if ( !burning )
	{
		if ( effectSet.find(kEffectBurning) != effectSet.end() )
		{
			deleteEffect(kEffectBurning);
		}
	}

	bool hungerIconActive = (effectSet.find(kEffectBread) != effectSet.end() 
		|| effectSet.find(kEffectBloodHunger) != effectSet.end()
		|| effectSet.find(kEffectAutomatonHunger) != effectSet.end());

	auto automatonHungerFrame = statusEffectFrame->findFrame("automaton hunger notification");
	automatonHungerFrame->setDisabled(true);
	auto automatonFlameImg = automatonHungerFrame->findImage("flame");

	int iconSize = 32;
	int movex = 0;
	if ( hungerIconActive )
	{
		if ( effectSet.find(kEffectBread) != effectSet.end() )
		{
			movex = 76;
		}
		else
		{
			movex = 64;
		}
		movex += 4;
	}
	const int startrowx = movex;
	int movey = statusEffectFrame->getSize().h - iconSize;
	const int spacing = 36;
	int numEffectsOnLine = 0;

	auto notificationFrame = statusEffectFrame->findFrame("notification frame");
	notificationFrame->setDisabled(true);
	auto notificationImg = notificationFrame->findImage("notification img");
	notificationImg->disabled = true;
	auto notificationTxt = notificationFrame->findField("notification txt");
	notificationTxt->setDisabled(true);
	notificationTxt->setFont(StatusEffectDefinitions_t::notificationFont.c_str());
	notificationTxt->setColor(StatusEffectDefinitions_t::notificationTextColor);
	if ( notificationQueue.size() >= 1 )
	{
		auto& notif = notificationQueue.front();
		notif.animateNotification(player);
		updateEntryImage(notif, notificationImg);
		if ( notif.notificationState == StatusEffectQueueEntry_t::STATE_2
			|| notif.notificationState == StatusEffectQueueEntry_t::STATE_3 )
		{
			if ( notif.effect >= kSpellEffectOffset )
			{
				int effectID = notif.effect - kSpellEffectOffset;
				if ( StatusEffectDefinitions_t::sustainedSpellDefinitionExists(effectID) )
				{
					auto& definition = StatusEffectDefinitions_t::getSustainedSpell(effectID);
					if ( notif.notificationState == StatusEffectQueueEntry_t::STATE_2 )
					{
						int variation = -1;
						if ( effectID == SPELL_SHADOW_TAG )
						{
							variation = 0;
							notificationTxt->setText("");
							if ( players[player] && players[player]->entity )
							{
								if ( players[player]->entity->creatureShadowTaggedThisUid != 0
									&& uidToEntity(players[player]->entity->creatureShadowTaggedThisUid) )
								{
									std::string formatString = definition.getName(variation).c_str();
									char buf[256] = "";
									Entity* tagged = uidToEntity(players[player]->entity->creatureShadowTaggedThisUid);
									if ( tagged->behavior == &actMonster )
									{
										int type = tagged->getMonsterTypeFromSprite();
										if ( type != NOTHING )
										{
											snprintf(buf, 1023, formatString.c_str(), getMonsterLocalizedName((Monster)type).c_str());
										}
										else
										{
											strcpy(buf, "");
										}
									}
									else if ( tagged->behavior == &actPlayer )
									{
										snprintf(buf, 1023, formatString.c_str(), stats[tagged->skill[2]]->name);
									}
									std::string formattedName = buf;
									notificationTxt->setText(formattedName.c_str());
								}
							}
						}
						else
						{
							notificationTxt->setText(definition.getName(variation).c_str());
						}
					}
					notificationTxt->setDisabled(false);
				}
			}
			else
			{
				int effectID = notif.effect;
				if ( StatusEffectDefinitions_t::effectDefinitionExists(effectID) )
				{
					auto& definition = StatusEffectDefinitions_t::getEffect(effectID);
					if ( notif.notificationState == StatusEffectQueueEntry_t::STATE_2 )
					{
						int variation = -1;
						if ( effectID == EFF_SHAPESHIFT )
						{
							if ( players[player] && players[player]->entity )
							{
								switch ( players[player]->entity->effectShapeshift )
								{
									case RAT:
										variation = 0;
										break;
									case SPIDER:
										variation = 1;
										break;
									case TROLL:
										variation = 2;
										break;
									case CREATURE_IMP:
										variation = 3;
										break;
									default:
										break;
								}
							}
						}
						else if ( effectID == StatusEffectQueue_t::kEffectBread
							|| effectID == StatusEffectQueue_t::kEffectBloodHunger )
						{
							if ( notif.customVariable >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_OVERSATIATED) )
							{
								variation = 0;
							}
							else if ( notif.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_STARVING) )
							{
								variation = 3;
							}
							else if ( notif.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_WEAK) )
							{
								variation = 2;
							}
							else if ( notif.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_HUNGRY) )
							{
								variation = 1;
							}
						}
						else if ( effectID == StatusEffectQueue_t::kEffectAutomatonHunger )
						{
							if ( notif.customVariable >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_SUPERHEATED) )
							{
								variation = 0;
							}
							else if ( notif.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
							{
								variation = 2;
							}
							else
							{
								variation = 1;
							}
						}
						else if ( effectID == EFF_VAMPIRICAURA )
						{
							bool sustained = false;
							for ( node_t* node = channeledSpells[player].first; node != nullptr; node = node->next )
							{
								spell_t* spell = (spell_t*)node->element;
								if ( spell && spell->ID == SPELL_VAMPIRIC_AURA )
								{
									sustained = true;
									break;
								}
							}
							if ( sustained )
							{
								variation = 1;
							}
							else
							{
								variation = 0;
							}
						}
						notificationTxt->setText(definition.getName(variation).c_str());
					}
					notificationTxt->setDisabled(false);
				}
			}
			if ( auto textGet = notificationTxt->getTextObject() )
			{
				SDL_Rect txtPos;
				txtPos.w = textGet->getWidth();
				txtPos.h = textGet->getHeight() * textGet->getNumTextLines() + 4;

				const int movementAmount = getStatusEffectMovementAmount(player);
				txtPos.x = (getBaseEffectPosX() - movementAmount - (notif.getStatusEffectLargestScaling(player) - 1.0) * notif.getEffectSpriteNormalWidth() / 2)
					+ (notif.getEffectSpriteNormalWidth() * notif.getStatusEffectLargestScaling(player)) / 2 - txtPos.w / 2;
				txtPos.y = getBaseEffectPosY() - movementAmount - txtPos.h;
				notificationTxt->setSize(txtPos);
			}
		}
		if ( notif.notificationState == StatusEffectQueueEntry_t::STATE_END )
		{
			notificationQueue.pop_front();
		}

		if ( notif.effect == kEffectAutomatonHunger )
		{
			automatonHungerFrame->setDisabled(false);
			automatonHungerFrame->setSize(notificationImg->pos);
		}
	}

	if ( !notificationImg->disabled || !notificationTxt->isDisabled() )
	{
		notificationFrame->setDisabled(false);
		notificationFrame->setSize(SDL_Rect{ 0, 0, statusEffectFrame->getSize().w, statusEffectFrame->getSize().h });
	}


	auto innerFrame = statusEffectFrame->findFrame("effects");
	int numFrameImages = innerFrame->getImages().size();
	while ( effectQueue.size() > numFrameImages )
	{
		auto img = innerFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "inner img");
		img->disabled = true;
		numFrameImages = innerFrame->getImages().size();
	}
	while ( effectQueue.size() < numFrameImages )
	{
		innerFrame->getImages().erase(innerFrame->getImages().begin());
		numFrameImages = innerFrame->getImages().size();
	}
	auto& frameImages = innerFrame->getImages();
	for ( auto img : frameImages )
	{
		img->disabled = true;
	}

	auto frameImagesIterator = frameImages.begin();
	bool bFrameCapturesMouse = false;
	if ( !players[player]->shootmode && inputs.getVirtualMouse(player)->draw_cursor 
		&& !inputs.getUIInteraction(player)->selectedItem && !players[player]->GUI.isDropdownActive() )
	{
		bFrameCapturesMouse = statusEffectFrame->capturesMouse();
	}
	bool tooltipShowing = false;
	bool bStatusEffectModuleActive = players[player]->GUI.activeModule == Player::GUI_t::MODULE_STATUS_EFFECTS;
	Sint32 mousex = (inputs.getMouse(player, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (inputs.getMouse(player, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
	bool lowDurationFlash = !((ticks % 50) - (ticks % 25));

	for ( auto it = effectQueue.rbegin(); it != effectQueue.rend(); )
	{
		auto& q = (*it);

		Frame::image_t* frameImg = nullptr;
		if ( frameImagesIterator != frameImages.end() )
		{
			frameImg = *frameImagesIterator;
		}

		bool existsInNotifications = false;
		for ( auto it2 = notificationQueue.begin(); it2 != notificationQueue.end(); ++it2 )
		{
			if ( (*it2).effect == q.effect )
			{
				existsInNotifications = true;
				break;
			}
		}
		if ( !existsInNotifications )
		{
			updateEntryImage(q, frameImg);

			if ( q.effect == kEffectAutomatonHunger )
			{
				automatonHungerFrame->setDisabled(false);
				automatonHungerFrame->setSize(frameImg->pos);
			}

			if ( bFrameCapturesMouse && !tooltipShowing )
			{
				SDL_Rect size = statusEffectFrame->getAbsoluteSize();
				int mouseDetectionPadding = 2;
				size.x += frameImg->pos.x - (mouseDetectionPadding);
				size.y += frameImg->pos.y - (mouseDetectionPadding);
				size.w = frameImg->pos.w + (mouseDetectionPadding * 2);
				size.h = frameImg->pos.h + (mouseDetectionPadding * 2);
				if ( rectContainsPoint(size, mousex, mousey) )
				{
					if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_STATUS_EFFECTS )
					{
						tooltipShowing = doStatusEffectTooltip(q, size);
					}
					players[player]->GUI.activateModule(Player::GUI_t::MODULE_STATUS_EFFECTS);
					players[player]->hud.setCursorDisabled(false);
					players[player]->hud.updateCursorAnimation(size.x - 1 + mouseDetectionPadding, size.y - 1 + mouseDetectionPadding, 
						frameImg->pos.w, frameImg->pos.h, inputs.getVirtualMouse(player)->draw_cursor);
				}
			}
		}
		int animatePosX = movex;
		int animatePosY = movey;
		if ( q.effect == kEffectBread || q.effect == kEffectBloodHunger )
		{
			animatePosX = 0;
			animatePosY = statusEffectFrame->getSize().h - q.getEffectSpriteNormalHeight();
		}
		else if ( q.effect == kEffectAutomatonHunger )
		{
			animatePosX = 0;
			animatePosY = 4 + statusEffectFrame->getSize().h - q.getEffectSpriteNormalHeight();
		}

		// low duration flash
		bool effectIsSustained = false;
		if ( StatusEffectDefinitions_t::effectDefinitionExists(q.effect) )
		{
			auto& definition = StatusEffectDefinitions_t::getEffect(q.effect);
			if ( definition.sustainedSpellID >= 0 )
			{
				if ( spellsActive.find(definition.sustainedSpellID) != spellsActive.end() )
				{
					effectIsSustained = true;
				}
			}
		}

		q.lowDuration = false;
		if ( !effectIsSustained && q.effect >= 0 && q.effect < NUMEFFECTS )
		{
			bool lowDuration = stats[player]->EFFECTS_TIMERS[q.effect] > 0 &&
				(stats[player]->EFFECTS_TIMERS[q.effect] < TICKS_PER_SECOND * 5);
			q.lowDuration = lowDuration;
			if ( lowDuration && lowDurationFlash )
			{
				frameImg->disabled = true;
			}
		}
		else if ( q.effect == kEffectBread || q.effect == kEffectBloodHunger || q.effect == kEffectAutomatonHunger )
		{
			if ( q.effect == kEffectAutomatonHunger )
			{
				if ( q.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
				{
					q.lowDuration = true;
					if ( lowDurationFlash )
					{
						frameImg->disabled = true;
						automatonHungerFrame->setDisabled(true);
					}
				}
			}
			else
			{
				if ( q.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_WEAK) )
				{
					q.lowDuration = true;
					if ( lowDurationFlash )
					{
						frameImg->disabled = true;
					}
				}
			}
		}
		else if ( q.effect == kEffectBurning )
		{
			q.lowDuration = true;
			if ( lowDurationFlash )
			{
				frameImg->disabled = true;
			}
		}

		if ( requiresAnimUpdate )
		{
			q.setAnimatePosition(animatePosX, animatePosY);
		}
		for ( auto it2 = notificationQueue.begin(); it2 != notificationQueue.end(); ++it2 )
		{
			if ( (*it2).effect == q.effect )
			{
				(*it2).notificationTargetPosition.x = animatePosX;
				(*it2).notificationTargetPosition.y = animatePosY;
				break;
			}
		}

		++it;
		if ( frameImagesIterator != frameImages.end() )
		{
			++frameImagesIterator;
		}

		q.animate();
		if ( q.effect == kEffectBread || q.effect == kEffectBloodHunger || q.effect == kEffectAutomatonHunger )
		{
			continue; // don't advance position as this is fixed
		}
		movex += spacing;
		++numEffectsOnLine;
		if ( numEffectsOnLine >= effectsPerRow )
		{
			numEffectsOnLine = 0;
			movex = startrowx;
			movey -= spacing;
		}
	}

	if ( stats[player] && stats[player]->type == AUTOMATON && !automatonHungerFrame->isDisabled() )
	{
		SDL_Rect automatonHungerFramePos = automatonHungerFrame->getSize();
		SDL_Rect boilerFlamePos = automatonHungerFramePos;
		boilerFlamePos.x = 0;
		boilerFlamePos.y = 0;
		if ( stats[player]->HUNGER > 300 )
		{
			if ( stats[player]->HUNGER > 1200 )
			{
				automatonFlameImg->path = "images/system/Hunger_boiler_hotfire.png";
				automatonFlameImg->pos = boilerFlamePos;
			}
			else
			{
				automatonFlameImg->path = "images/system/Hunger_boiler_fire.png";
				if ( stats[player]->HUNGER > 600 )
				{
					automatonFlameImg->pos = boilerFlamePos;
				}
				else
				{
					float percent = (stats[player]->HUNGER - 200) / 400.f; // always show a little bit more at the bottom (10-20%)
					automatonFlameImg->pos = boilerFlamePos;
					int newHeight = boilerFlamePos.h * percent;
					int heightDiff = boilerFlamePos.h - newHeight;
					automatonFlameImg->pos.y -= heightDiff;
					automatonHungerFramePos.y += heightDiff;
					automatonHungerFramePos.h -= heightDiff;
					automatonHungerFrame->setSize(automatonHungerFramePos);
				}
			}
		}
		else
		{
			automatonHungerFrame->setDisabled(true);
		}
	}

	animateStatusEffectTooltip(tooltipShowing);

	if ( bStatusEffectModuleActive )
	{
		if ( !tooltipShowing )
		{
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_NONE);
		}
	}

	requiresAnimUpdate = false;
}

void StatusEffectQueue_t::updateEntryImage(StatusEffectQueueEntry_t& entry, Frame::image_t* img)
{
	if ( img )
	{
		img->path = "";
		if ( entry.effect == kEffectBread || entry.effect == kEffectBloodHunger )
		{
			if ( StatusEffectDefinitions_t::effectDefinitionExists(entry.effect) )
			{
				int variation = -1;
				if ( entry.customVariable >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_OVERSATIATED) )
				{
					variation = 0;
				}
				else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_STARVING) )
				{
					variation = 3;
				}
				else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_WEAK) )
				{
					variation = 2;
				}
				else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_HUNGRY) )
				{
					variation = 1;
				}
				if ( variation >= 0 )
				{
					img->path = StatusEffectDefinitions_t::getEffectImgPath(StatusEffectDefinitions_t::getEffect(entry.effect), variation);
				}
				else
				{
					img->path = "";
				}
			}
		}
		else if ( entry.effect == kEffectAutomatonHunger )
		{
			int variation = 1;
			if ( entry.customVariable >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_SUPERHEATED) )
			{
				variation = 0;
			}
			else if ( entry.customVariable <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_CRITICAL) )
			{
				variation = 2;
			}
			img->path = StatusEffectDefinitions_t::getEffectImgPath(StatusEffectDefinitions_t::getEffect(entry.effect), variation);
		}
		else
		{
			if ( entry.effect >= kSpellEffectOffset )
			{
				int effectID = entry.effect - kSpellEffectOffset;
				if ( StatusEffectDefinitions_t::sustainedSpellDefinitionExists(effectID) )
				{
					img->path = StatusEffectDefinitions_t::getEffectImgPath(StatusEffectDefinitions_t::getSustainedSpell(effectID));
				}
			}
			else
			{
				int effectID = entry.effect;
				if ( StatusEffectDefinitions_t::effectDefinitionExists(effectID) )
				{
					int variation = -1;
					if ( effectID == EFF_SHAPESHIFT )
					{
						if ( players[player] && players[player]->entity )
						{
							switch ( players[player]->entity->effectShapeshift )
							{
								case RAT:
									variation = 0;
									break;
								case SPIDER:
									variation = 1;
									break;
								case TROLL:
									variation = 2;
									break;
								case CREATURE_IMP:
									variation = 3;
									break;
								default:
									break;
							}
						}
					}
					img->path = StatusEffectDefinitions_t::getEffectImgPath(StatusEffectDefinitions_t::getEffect(effectID), variation);
				}
			}
		}

		SDL_Rect dest{ 0, 0, entry.pos.w, entry.pos.h };
		dest.x = entry.pos.x;
		dest.y = entry.pos.y;
		img->pos = dest;
		img->disabled = false;

		if ( img->path.size() > 1 && img->path[0] != '*' )
		{
			img->path.insert(0, 1, '*');
		}
	}
}

void updateStatusEffectQueue(const int player)
{
	auto& statusEffectQueue = StatusEffectQueue[player];
	if ( !statusEffectQueue.statusEffectFrame )
	{
		return;
	}
	auto& hud_t = players[player]->hud;
	SDL_Rect mainFramePos{ 0, 0, players[player]->camera_virtualWidth(), players[player]->camera_virtualHeight() / 2 };
	mainFramePos.x = hud_t.hpFrame->getSize().x;
	mainFramePos.y = hud_t.hpFrame->getSize().y - mainFramePos.h;
	mainFramePos.w -= mainFramePos.x;
	statusEffectQueue.statusEffectFrame->setSize(mainFramePos);
	auto innerFrame = statusEffectQueue.statusEffectFrame->findFrame("effects");
	innerFrame->setSize(SDL_Rect{ 0, 0, statusEffectQueue.statusEffectFrame->getSize().w, statusEffectQueue.statusEffectFrame->getSize().h });

	const int hungerEffectID = ((stats[player] && stats[player]->type == AUTOMATON) ? StatusEffectQueue_t::kEffectAutomatonHunger
		: (playerRequiresBloodToSustain(player) ? StatusEffectQueue_t::kEffectBloodHunger : StatusEffectQueue_t::kEffectBread));

	// hunger icon
	if ( stats[player] && stats[player]->type != AUTOMATON )
	{
		statusEffectQueue.deleteEffect(StatusEffectQueue_t::kEffectAutomatonHunger);
	}

	if ( stats[player] && stats[player]->type != AUTOMATON
		&& (svFlags & SV_FLAG_HUNGER) 
		&& (stats[player]->HUNGER <= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_HUNGRY) 
			|| stats[player]->HUNGER >= getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_OVERSATIATED)) )
	{
		statusEffectQueue.deleteEffect(StatusEffectQueue_t::kEffectAutomatonHunger);
		if ( hungerEffectID == StatusEffectQueue_t::kEffectBloodHunger )
		{
			statusEffectQueue.deleteEffect(StatusEffectQueue_t::kEffectBread); // delete opposite if present
		}
		if ( hungerEffectID == StatusEffectQueue_t::kEffectBread )
		{
			statusEffectQueue.deleteEffect(StatusEffectQueue_t::kEffectBloodHunger); // delete opposite if present
		}

		const int HUNGER_NONE = 1000;
		const int HUNGER_OVERSATIATED = getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_OVERSATIATED);
		const int HUNGER_HUNGRY = getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_HUNGRY);
		const int HUNGER_WEAK = getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_WEAK);
		const int HUNGER_STARVING = getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_STARVING);
		int hungerStateToSet = HUNGER_NONE;
		if ( stats[player]->HUNGER >= HUNGER_OVERSATIATED )
		{
			hungerStateToSet = HUNGER_OVERSATIATED;
		}
		else if ( stats[player]->HUNGER <= HUNGER_STARVING )
		{
			hungerStateToSet = HUNGER_STARVING;
		}
		else if ( stats[player]->HUNGER <= HUNGER_WEAK )
		{
			hungerStateToSet = HUNGER_WEAK;
		}
		else if ( stats[player]->HUNGER <= HUNGER_HUNGRY )
		{
			hungerStateToSet = HUNGER_HUNGRY;
		}
		StatusEffectQueueEntry_t* entry = nullptr;
		StatusEffectQueueEntry_t* notif = nullptr;


		for ( auto& q : statusEffectQueue.effectQueue )
		{
			if ( q.effect == hungerEffectID )
			{
				entry = &(q);
				for ( auto& n : statusEffectQueue.notificationQueue )
				{
					if ( n.effect == hungerEffectID )
					{
						notif = &(n);
					}
					break;
				}
				break;
			}
		}
		if ( entry && entry->customVariable != hungerStateToSet )
		{
			if ( notif && notif->customVariable != hungerStateToSet )
			{
				// reset the notification
				bool erased = false;
				for ( auto it = statusEffectQueue.notificationQueue.begin(); it != statusEffectQueue.notificationQueue.end(); ++it )
				{
					if ( (*it).effect == hungerEffectID )
					{
						statusEffectQueue.notificationQueue.erase(it);
						erased = true;
						break;
					}
				}
				statusEffectQueue.notificationQueue.push_back(StatusEffectQueueEntry_t(hungerEffectID));
				statusEffectQueue.notificationQueue.back().pos.x = statusEffectQueue.getBaseEffectPosX();
				statusEffectQueue.notificationQueue.back().pos.y = statusEffectQueue.getBaseEffectPosY();

				// fall back if notificationTargetPosition doesn't have an effect to go to.
				statusEffectQueue.notificationQueue.back().notificationTargetPosition.x = 0;
				statusEffectQueue.notificationQueue.back().notificationTargetPosition.y = statusEffectQueue.statusEffectFrame->getSize().h 
					- statusEffectQueue.notificationQueue.back().notificationTargetPosition.h;
				statusEffectQueue.requiresAnimUpdate = true;
				entry->customVariable = hungerStateToSet;
				statusEffectQueue.notificationQueue.back().customVariable = hungerStateToSet;
			}
			else
			{
				// else, delete and reapply
				statusEffectQueue.deleteEffect(hungerEffectID);
				if ( statusEffectQueue.insertEffect(hungerEffectID, -1) )
				{
					if ( statusEffectQueue.effectQueue.back().effect == hungerEffectID )
					{
						statusEffectQueue.effectQueue.back().customVariable = hungerStateToSet;
						statusEffectQueue.notificationQueue.back().customVariable = hungerStateToSet;
					}
				}
			}
		}
		else
		{
			// new effect, does not exist
			if ( statusEffectQueue.insertEffect(hungerEffectID, -1) )
			{
				if ( statusEffectQueue.effectQueue.back().effect == hungerEffectID )
				{
					statusEffectQueue.effectQueue.back().customVariable = hungerStateToSet;
					statusEffectQueue.notificationQueue.back().customVariable = hungerStateToSet;
				}
			}
		}
	}
	else
	{
		statusEffectQueue.deleteEffect(StatusEffectQueue_t::kEffectBloodHunger);
		statusEffectQueue.deleteEffect(StatusEffectQueue_t::kEffectBread);

		if ( hungerEffectID == StatusEffectQueue_t::kEffectAutomatonHunger )
		{
			const int HUNGER_NONE = 1000;
			const int HUNGER_SUPERHEATED = getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_SUPERHEATED);
			const int HUNGER_CRITICAL = getEntityHungerInterval(player, nullptr, stats[player], HUNGER_INTERVAL_AUTOMATON_CRITICAL);
			int hungerStateToSet = HUNGER_NONE;
			if ( stats[player]->HUNGER >= HUNGER_SUPERHEATED )
			{
				hungerStateToSet = HUNGER_SUPERHEATED;
			}
			else if ( stats[player]->HUNGER <= HUNGER_CRITICAL )
			{
				hungerStateToSet = HUNGER_CRITICAL;
			}
			StatusEffectQueueEntry_t* entry = nullptr;
			StatusEffectQueueEntry_t* notif = nullptr;

			for ( auto& q : statusEffectQueue.effectQueue )
			{
				if ( q.effect == hungerEffectID )
				{
					entry = &(q);
					for ( auto& n : statusEffectQueue.notificationQueue )
					{
						if ( n.effect == hungerEffectID )
						{
							notif = &(n);
						}
						break;
					}
					break;
				}
			}
			if ( entry && entry->customVariable != hungerStateToSet )
			{
				if ( notif && notif->customVariable != hungerStateToSet )
				{
					// reset the notification
					bool erased = false;
					for ( auto it = statusEffectQueue.notificationQueue.begin(); it != statusEffectQueue.notificationQueue.end(); ++it )
					{
						if ( (*it).effect == hungerEffectID )
						{
							statusEffectQueue.notificationQueue.erase(it);
							erased = true;
							break;
						}
					}
					statusEffectQueue.notificationQueue.push_back(StatusEffectQueueEntry_t(hungerEffectID));
					statusEffectQueue.notificationQueue.back().pos.x = statusEffectQueue.getBaseEffectPosX();
					statusEffectQueue.notificationQueue.back().pos.y = statusEffectQueue.getBaseEffectPosY();

					// fall back if notificationTargetPosition doesn't have an effect to go to.
					statusEffectQueue.notificationQueue.back().notificationTargetPosition.x = 0;
					statusEffectQueue.notificationQueue.back().notificationTargetPosition.y = statusEffectQueue.statusEffectFrame->getSize().h
						- statusEffectQueue.notificationQueue.back().notificationTargetPosition.h;
					statusEffectQueue.requiresAnimUpdate = true;
					entry->customVariable = hungerStateToSet;
					statusEffectQueue.notificationQueue.back().customVariable = hungerStateToSet;
				}
				else
				{
					// else, delete and reapply
					statusEffectQueue.deleteEffect(hungerEffectID);
					if ( statusEffectQueue.insertEffect(hungerEffectID, -1) )
					{
						if ( statusEffectQueue.effectQueue.back().effect == hungerEffectID )
						{
							statusEffectQueue.effectQueue.back().customVariable = hungerStateToSet;
							statusEffectQueue.notificationQueue.back().customVariable = hungerStateToSet;
						}
					}
				}
			}
			else
			{
				// new effect, does not exist
				if ( statusEffectQueue.insertEffect(hungerEffectID, -1) )
				{
					if ( statusEffectQueue.effectQueue.back().effect == hungerEffectID )
					{
						statusEffectQueue.effectQueue.back().customVariable = hungerStateToSet;
						statusEffectQueue.notificationQueue.back().customVariable = hungerStateToSet;
					}
				}
			}
		}
	}

	statusEffectQueue.updateAllQueuedEffects();
}

void createWorldTooltipPrompts(const int player)
{
	auto& hud_t = players[player]->hud;
	auto& worldTooltipFrame = hud_t.worldTooltipFrame;
	worldTooltipFrame = hud_t.hudFrame->addFrame("world tooltip");
	worldTooltipFrame->setHollow(true);
	worldTooltipFrame->setBorder(0);
	worldTooltipFrame->setOwner(player);
	worldTooltipFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });
	worldTooltipFrame->setDisabled(true);

	const char* promptFont = "fonts/pixel_maz_multiline.ttf#16#2";

	Uint32 iconColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconOpacity);
	Uint32 iconBackingColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconBackingOpacity);

	auto text = worldTooltipFrame->addField("prompt text", 256);
	text->setFont(promptFont);
	text->setText("");
	text->setDisabled(true);
	text->setSize(SDL_Rect{ 0, 0, 0, 0 });

	const int iconSize = 24;
	SDL_Rect iconPos{ 0, 0, iconSize, iconSize };

	auto icon = worldTooltipFrame->addImage(iconPos,
		iconColor, "images/system/white.png", "icon img");
	icon->disabled = true;

	auto glyph = worldTooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "images/system/white.png", "glyph img");
	glyph->disabled = true;

	auto glyphAdditional = worldTooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "images/system/white.png", "glyph img 2");
	glyphAdditional->disabled = true;

	auto cursor = worldTooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "images/system/white.png", "cursor img");
	cursor->disabled = true;
}

void Player::HUD_t::updateWorldTooltipPrompts()
{
	if ( !hudFrame )
	{
		return;
	}

	if ( !worldTooltipFrame )
	{
		createWorldTooltipPrompts(player.playernum);
		if ( !worldTooltipFrame )
		{
			return;
		}
	}

	worldTooltipFrame->setDisabled(true);

	if ( !player.shootmode || nohud || gamePaused )
	{
		return;
	}

	SDL_Rect promptPos{ player.camera_virtualWidth() / 2, player.camera_virtualHeight() / 2, 0, 0 };

	FollowerRadialMenu& followerMenu = FollowerMenu[player.playernum];

	auto icon = worldTooltipFrame->findImage("icon img");
	icon->disabled = true;
	auto glyph = worldTooltipFrame->findImage("glyph img");
	glyph->disabled = true;
	auto glyphAdditional = worldTooltipFrame->findImage("glyph img 2");
	glyphAdditional->disabled = true;
	auto cursor = worldTooltipFrame->findImage("cursor img");
	cursor->disabled = true;
	auto text = worldTooltipFrame->findField("prompt text");
	text->setDisabled(true);

	SDL_Rect textPos{ 0, 0, 0, 0 };
	const int skillIconToGlyphPadding = 4;
	const int nominalGlyphHeight = 26;

	if ( followerMenu.selectMoveTo && (followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT
		|| followerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT) )
	{
		bool forceBlankInteractText = false;
		if ( !player.worldUI.isEnabled() )
		{
			cursor->path = "images/system/cursor.png";
			if ( auto imgGet = Image::get(cursor->path.c_str()) )
			{
				cursor->disabled = false;
				promptPos.x -= (int)imgGet->getWidth() / 2;
				promptPos.y -= (int)imgGet->getHeight() / 2;
				SDL_Rect cursorPos{ 0, 0, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
				cursor->pos = cursorPos;
				cursor->color = makeColor(255, 255, 255, 191);
			}
			textPos.x = cursor->pos.x + 24;
			textPos.y = cursor->pos.y + 20;
		}
		else
		{
			if ( followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT )
			{
				cursor->path = "images/system/cursor.png";
				if ( auto imgGet = Image::get(cursor->path.c_str()) )
				{
					cursor->disabled = false;
					promptPos.x -= (int)imgGet->getWidth() / 2;
					promptPos.y -= (int)imgGet->getHeight() / 2;
					SDL_Rect cursorPos{ 0, 0, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
					cursor->pos = cursorPos;
					cursor->color = makeColor(255, 255, 255, 191);
				}

				textPos.x = cursor->pos.x + cursor->pos.w / 2;
				textPos.y = cursor->pos.y + cursor->pos.h / 2;
				if ( auto imgGet = Image::get("images/system/selectedcursor.png") )
				{
					textPos.x -= (int)imgGet->getWidth() / 2;
					textPos.y -= (int)imgGet->getHeight() / 2;
				}

				if ( textPos.x < 0 )
				{
					promptPos.x += textPos.x;
					cursor->pos.x -= textPos.x;
					textPos.x = 0;
				}
				if ( textPos.y < 0 )
				{
					promptPos.y += textPos.y;
					cursor->pos.y -= textPos.y;
					textPos.y = 0;
				}
			}
			else
			{
				if ( (player.worldUI.isEnabled() && !player.worldUI.bTooltipInView) )
				{
					forceBlankInteractText = true;

					cursor->path = "*#images/system/cross.png";
					if ( auto imgGet = Image::get(cursor->path.c_str()) )
					{
						cursor->disabled = false;
						promptPos.x -= (int)imgGet->getWidth() / 2;
						promptPos.y -= (int)imgGet->getHeight() / 2;
						SDL_Rect cursorPos{ 0, 0, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
						cursor->pos = cursorPos;
						cursor->color = makeColor(255, 255, 255, 128);
					}

					textPos.x = cursor->pos.x + cursor->pos.w / 2;
					textPos.y = cursor->pos.y + cursor->pos.h / 2;
					if ( auto imgGet = Image::get("images/system/selectedcursor.png") )
					{
						textPos.x -= (int)imgGet->getWidth() / 2;
						textPos.y -= (int)imgGet->getHeight() / 2;
					}

					if ( textPos.x < 0 )
					{
						promptPos.x += textPos.x;
						cursor->pos.x -= textPos.x;
						textPos.x = 0;
					}
					if ( textPos.y < 0 )
					{
						promptPos.y += textPos.y;
						cursor->pos.y -= textPos.y;
						textPos.y = 0;
					}
				}
				else
				{
					cursor->path = "images/system/selectedcursor.png";
					if ( auto imgGet = Image::get(cursor->path.c_str()) )
					{
						cursor->disabled = false;
						promptPos.x -= (int)imgGet->getWidth() / 2;
						promptPos.y -= (int)imgGet->getHeight() / 2;
						SDL_Rect cursorPos{ 0, 0, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
						cursor->pos = cursorPos;
						cursor->color = makeColor(255, 255, 255, 128);
					}
				}
			}

			textPos.x += 40;
			textPos.y += 20;
		}

		auto glyphPathPressed = Input::inputs[player.playernum].getGlyphPathForBinding("Use", true);
		auto glyphPathUnpressed = Input::inputs[player.playernum].getGlyphPathForBinding("Use", false);
		auto glyphAdditionalPathPressed = Input::inputs[player.playernum].getGlyphPathForBinding("Block", true);
		auto glyphAdditionalPathUnpressed = Input::inputs[player.playernum].getGlyphPathForBinding("Block", false);
		if ( ticks % 50 < 25 )
		{
			glyph->path = glyphPathPressed;
			if ( auto imgGet = Image::get(glyph->path.c_str()) )
			{
				glyph->disabled = false;
				SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
				glyph->pos = glyphPos;
				if ( auto imgGetUnpressed = Image::get(glyphPathUnpressed.c_str()) )
				{
					const int unpressedHeight = imgGetUnpressed->getHeight();
					if ( unpressedHeight != glyph->pos.h )
					{
						glyph->pos.y -= (glyph->pos.h - unpressedHeight);
					}

					if ( unpressedHeight != nominalGlyphHeight )
					{
						glyph->pos.y -= (unpressedHeight - nominalGlyphHeight) / 2;
					}
				}
				textPos.x += glyph->pos.w;
			}
		}
		else
		{
			glyph->path = glyphPathUnpressed;
			if ( auto imgGet = Image::get(glyph->path.c_str()) )
			{
				glyph->disabled = false;
				SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
				glyph->pos = glyphPos;
				textPos.x += glyph->pos.w;

				if ( glyph->pos.h != nominalGlyphHeight )
				{
					glyph->pos.y -= (glyph->pos.h - nominalGlyphHeight) / 2;
				}
			}
		}

		textPos.x += skillIconToGlyphPadding;

		if ( Input::inputs[player.playernum].binary("Block") )
		{
			glyphAdditional->path = glyphAdditionalPathUnpressed;
			if ( auto imgGet = Image::get(glyphAdditional->path.c_str()) )
			{
				glyphAdditional->disabled = false;
				SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
				glyphAdditional->pos = glyphPos;
				if ( auto imgGetUnpressed = Image::get(glyphAdditionalPathUnpressed.c_str()) )
				{
					const int unpressedHeight = imgGetUnpressed->getHeight();
					if ( unpressedHeight != glyphAdditional->pos.h )
					{
						glyphAdditional->pos.y -= (glyphAdditional->pos.h - unpressedHeight);
					}

					if ( unpressedHeight != nominalGlyphHeight )
					{
						glyphAdditional->pos.y -= (unpressedHeight - nominalGlyphHeight) / 2;
					}
				}
				textPos.x += glyphAdditional->pos.w;
				textPos.x += skillIconToGlyphPadding;
			}
		}

		for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
		{
			if ( skill.skillId == PRO_LEADERSHIP )
			{
				if ( skillCapstoneUnlocked(player.playernum, PRO_LEADERSHIP) )
				{
					icon->path = skill.skillIconPathLegend;
				}
				else
				{
					icon->path = skill.skillIconPath;
				}
				break;
			}
		}

		if ( auto imgGet = Image::get(icon->path.c_str()) )
		{
			icon->disabled = false;
			SDL_Rect iconPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
			icon->pos = iconPos;
			textPos.x += icon->pos.w + skillIconToGlyphPadding;
		}
		

		if ( followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT )
		{
			if ( followerMenu.followerToCommand
				&& (followerMenu.followerToCommand->getMonsterTypeFromSprite() == SENTRYBOT
					|| followerMenu.followerToCommand->getMonsterTypeFromSprite() == SPELLBOT)
				)
			{
				text->setDisabled(false);
				if ( !glyphAdditional->disabled )
				{
					std::string txt = language[4201];
					txt += language[3650];
					text->setText(txt.c_str());
				}
				else
				{
					text->setText(language[3650]);
				}
			}
			else
			{
				text->setDisabled(false);
				if ( !glyphAdditional->disabled )
				{
					std::string txt = language[4201];
					txt += language[3039];
					text->setText(txt.c_str());
				}
				else
				{
					text->setText(language[3039]);
				}
			}
		}
		else
		{
			if ( !strcmp(followerMenu.interactText, "") || forceBlankInteractText )
			{
				if ( followerMenu.followerToCommand )
				{
					int type = followerMenu.followerToCommand->getMonsterTypeFromSprite();
					if ( followerMenu.allowedInteractItems(type)
						|| followerMenu.allowedInteractFood(type)
						|| followerMenu.allowedInteractWorld(type)
						)
					{
						text->setDisabled(false);
						text->setText(language[4041]); // "Interact with..."
					}
					else
					{
						text->setDisabled(false);
						text->setText(language[4042]); // "Attack..."
					}
				}
				else
				{
					text->setDisabled(false);
					text->setText(language[4041]); // "Interact with..."
				}
			}
			else
			{
				text->setDisabled(false);
				text->setText(followerMenu.interactText);
			}
		}

		if ( !glyphAdditional->disabled )
		{
			if ( !strstr(text->getText(), language[4201]) ) // no "(ALL) " text
			{
				glyphAdditional->disabled = true;
				textPos.x -= glyphAdditional->pos.w + skillIconToGlyphPadding;
				icon->pos.x -= glyphAdditional->pos.w + skillIconToGlyphPadding;
				//textPos.x -= icon->pos.w + skillIconToGlyphPadding;
			}
		}
	}
	else
	{
		bool foundTinkeringKit = false;
		if ( player.entity && stats[player.playernum]
			&& stats[player.playernum]->defending )
		{
			if ( stats[player.playernum]->shield && stats[player.playernum]->shield->type == TOOL_TINKERING_KIT )
			{
				foundTinkeringKit = true;
			}
		}
		if ( player.worldUI.bTooltipInView )
		{
			cursor->path = "images/system/selectedcursor.png";
			if ( auto imgGet = Image::get(cursor->path.c_str()) )
			{
				cursor->disabled = false;
				promptPos.x -= (int)imgGet->getWidth() / 2;
				promptPos.y -= (int)imgGet->getHeight() / 2;
				SDL_Rect cursorPos{ 0, 0, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
				cursor->pos = cursorPos;
				cursor->color = makeColor(255, 255, 255, 128);
			}

			textPos.x += 40;
			textPos.y += 20;

			auto glyphPathPressed = Input::inputs[player.playernum].getGlyphPathForBinding("Use", true);
			auto glyphPathUnpressed = Input::inputs[player.playernum].getGlyphPathForBinding("Use", false);
			if ( ticks % 50 < 25 )
			{
				glyph->path = glyphPathPressed;
				if ( auto imgGet = Image::get(glyph->path.c_str()) )
				{
					glyph->disabled = false;
					SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
					glyph->pos = glyphPos;
					if ( auto imgGetUnpressed = Image::get(glyphPathUnpressed.c_str()) )
					{
						const int unpressedHeight = imgGetUnpressed->getHeight();
						if ( unpressedHeight != glyph->pos.h )
						{
							glyph->pos.y -= (glyph->pos.h - unpressedHeight);
						}

						if ( unpressedHeight != nominalGlyphHeight )
						{
							glyph->pos.y -= (unpressedHeight - nominalGlyphHeight) / 2;
						}
					}
					textPos.x += glyph->pos.w;
				}
			}
			else
			{
				glyph->path = glyphPathUnpressed;
				if ( auto imgGet = Image::get(glyph->path.c_str()) )
				{
					glyph->disabled = false;
					SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
					glyph->pos = glyphPos;
					textPos.x += glyph->pos.w;

					if ( glyph->pos.h != nominalGlyphHeight )
					{
						glyph->pos.y -= (glyph->pos.h - nominalGlyphHeight) / 2;
					}
				}
			}
			textPos.x += skillIconToGlyphPadding;

			if ( foundTinkeringKit )
			{
				for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
				{
					if ( skill.skillId == PRO_LOCKPICKING )
					{
						if ( skillCapstoneUnlocked(player.playernum, PRO_LOCKPICKING) )
						{
							icon->path = skill.skillIconPathLegend;
						}
						else
						{
							icon->path = skill.skillIconPath;
						}
						break;
					}
				}

				if ( auto imgGet = Image::get(icon->path.c_str()) )
				{
					icon->disabled = false;
					SDL_Rect iconPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
					icon->pos = iconPos;
					textPos.x += icon->pos.w + skillIconToGlyphPadding;
				}
			}

			text->setDisabled(false);
			text->setText(player.worldUI.interactText.c_str());
		}
		else
		{
			cursor->path = "*#images/system/cross.png";
			if ( auto imgGet = Image::get(cursor->path.c_str()) )
			{
				cursor->disabled = false;
				promptPos.x -= (int)imgGet->getWidth() / 2;
				promptPos.y -= (int)imgGet->getHeight() / 2;
				SDL_Rect cursorPos{ 0, 0, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
				cursor->pos = cursorPos;
				cursor->color = makeColor(255, 255, 255, 128);
			}

			if ( foundTinkeringKit )
			{
				if ( player.worldUI.isEnabled() )
				{
					textPos.x = cursor->pos.x + cursor->pos.w / 2;
					textPos.y = cursor->pos.y + cursor->pos.h / 2;
					if ( auto imgGet = Image::get("images/system/selectedcursor.png") )
					{
						textPos.x -= (int)imgGet->getWidth() / 2;
						textPos.y -= (int)imgGet->getHeight() / 2;
					}

					if ( textPos.x < 0 )
					{
						promptPos.x += textPos.x;
						cursor->pos.x -= textPos.x;
						textPos.x = 0;
					}
					if ( textPos.y < 0 )
					{
						promptPos.y += textPos.y;
						cursor->pos.y -= textPos.y;
						textPos.y = 0;
					}

					// skip cursor here, no tooltip in view
					//drawImageAlpha(selected_cursor_bmp, NULL, &pos, 128);
					textPos.x += 40;
					textPos.y += 20;

					auto glyphPathPressed = Input::inputs[player.playernum].getGlyphPathForBinding("Use", true);
					auto glyphPathUnpressed = Input::inputs[player.playernum].getGlyphPathForBinding("Use", false);
					if ( ticks % 50 < 25 )
					{
						glyph->path = glyphPathPressed;
						if ( auto imgGet = Image::get(glyph->path.c_str()) )
						{
							glyph->disabled = false;
							SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
							glyph->pos = glyphPos;
							if ( auto imgGetUnpressed = Image::get(glyphPathUnpressed.c_str()) )
							{
								const int unpressedHeight = imgGetUnpressed->getHeight();
								if ( unpressedHeight != glyph->pos.h )
								{
									glyph->pos.y -= (glyph->pos.h - unpressedHeight);
								}

								if ( unpressedHeight != nominalGlyphHeight )
								{
									glyph->pos.y -= (unpressedHeight - nominalGlyphHeight) / 2;
								}
							}
							textPos.x += glyph->pos.w;
						}
					}
					else
					{
						glyph->path = glyphPathUnpressed;
						if ( auto imgGet = Image::get(glyph->path.c_str()) )
						{
							glyph->disabled = false;
							SDL_Rect glyphPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
							glyph->pos = glyphPos;
							textPos.x += glyph->pos.w;

							if ( glyph->pos.h != nominalGlyphHeight )
							{
								glyph->pos.y -= (glyph->pos.h - nominalGlyphHeight) / 2;
							}
						}
					}
					textPos.x += skillIconToGlyphPadding;

					for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
					{
						if ( skill.skillId == PRO_LOCKPICKING )
						{
							if ( skillCapstoneUnlocked(player.playernum, PRO_LOCKPICKING) )
							{
								icon->path = skill.skillIconPathLegend;
							}
							else
							{
								icon->path = skill.skillIconPath;
							}
							break;
						}
					}

					if ( auto imgGet = Image::get(icon->path.c_str()) )
					{
						icon->disabled = false;
						SDL_Rect iconPos{ textPos.x, textPos.y, (int)imgGet->getWidth(), (int)imgGet->getHeight() };
						icon->pos = iconPos;
						textPos.x += icon->pos.w + skillIconToGlyphPadding;
					}

					text->setDisabled(false);
					text->setText(language[3663]);
				}
				else
				{
					textPos.x = cursor->pos.x + 15;
					textPos.y = cursor->pos.y + 11;

					text->setDisabled(false);
					text->setText(language[3663]);
				}
			}
		}
	}

	if ( !text->isDisabled() )
	{
		textPos.w = text->getTextObject()->getWidth();
		textPos.h = Font::get(text->getFont())->height() + 8;
		textPos.y -= 1;
		text->setVJustify(Field::justify_t::CENTER);
		text->setSize(textPos);

		promptPos.w = text->getSize().x + text->getSize().w;
		promptPos.h = text->getSize().y + text->getSize().h;
	}
	if ( !glyph->disabled )
	{
		promptPos.w = std::max(glyph->pos.x + glyph->pos.w, promptPos.w);
		promptPos.h = std::max(glyph->pos.y + glyph->pos.h, promptPos.h);
	}
	if ( !icon->disabled )
	{
		if ( icon->pos.h != nominalGlyphHeight )
		{
			icon->pos.y -= (icon->pos.h - nominalGlyphHeight) / 2;
		}

		promptPos.w = std::max(icon->pos.x + icon->pos.w, promptPos.w);
		promptPos.h = std::max(icon->pos.y + icon->pos.h, promptPos.h);
	}
	if ( !cursor->disabled )
	{
		promptPos.w = std::max(cursor->pos.x + cursor->pos.w, promptPos.w);
		promptPos.h = std::max(cursor->pos.y + cursor->pos.h, promptPos.h);
	}
	worldTooltipFrame->setSize(promptPos);
	worldTooltipFrame->setDisabled(false);
}

std::string actionPromptBackingIconPath00 = "";
std::string actionPromptBackingIconPath20 = "";
std::string actionPromptBackingIconPath60 = "";
std::string actionPromptBackingIconPath100 = "";

void createActionPrompts(const int player)
{
	auto& hud_t = players[player]->hud;
	auto& actionPromptFrame = hud_t.actionPromptsFrame;
	actionPromptFrame = hud_t.hudFrame->addFrame("action prompts");
	actionPromptFrame->setHollow(true);
	actionPromptFrame->setBorder(0);
	actionPromptFrame->setOwner(player);
	actionPromptFrame->setSize(SDL_Rect{ 0, 0, hud_t.hudFrame->getSize().w, hud_t.hudFrame->getSize().h });
	actionPromptFrame->setDisabled(true);

	const int iconSize = Player::HUD_t::actionPromptIconSize;
	const int glyphSize = 32;
	const int iconBackingSize = Player::HUD_t::actionPromptBackingSize;
	const int maxWidth = std::max(iconSize, iconBackingSize);
	const int maxHeight = std::max(iconSize, iconBackingSize);
	const int promptHeight = maxHeight + glyphSize; // vertical space for prompts

	SDL_Rect iconPos{ maxWidth / 2 - iconSize / 2 - 1, 
		promptHeight - maxHeight / 2 - iconSize / 2 - 1, 
		iconSize, 
		iconSize };
	SDL_Rect iconBackingPos{ maxWidth / 2 - iconBackingSize / 2, 
		promptHeight - maxHeight / 2 - iconBackingSize / 2, 
		iconBackingSize, 
		iconBackingSize };

	const char* promptFont = "fonts/pixel_maz_multiline.ttf#16#2";

	Uint32 iconColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconOpacity);
	Uint32 iconBackingColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconBackingOpacity);

	auto mainHand = actionPromptFrame->addFrame("action mainhand");
	mainHand->setSize(SDL_Rect{ 400, 400, maxWidth, promptHeight });
	mainHand->addImage(iconBackingPos,
		iconBackingColor, actionPromptBackingIconPath00.c_str(), "action img backing");
	mainHand->addImage(iconPos,
		iconColor, "images/system/white.png", "action img");
	Frame::image_t* glyph = actionPromptFrame->addImage(SDL_Rect{ 0, 0, mainHand->getSize().w, glyphSize },
		0xFFFFFFFF, "images/system/white.png", "action mainhand glyph");
	glyph->ontop = true;
	auto mainHandText = actionPromptFrame->addField("action mainhand text", 64);
	mainHandText->setFont(promptFont);
	mainHandText->setText("Attack");
	mainHandText->setHJustify(Field::justify_t::CENTER);

	auto offHand = actionPromptFrame->addFrame("action offhand");
	offHand->setSize(SDL_Rect{ 440, 400, maxWidth, promptHeight });
	offHand->addImage(iconBackingPos,
		iconBackingColor, actionPromptBackingIconPath00.c_str(), "action img backing");
	offHand->addImage(iconPos,
		iconColor, "images/system/white.png", "action img");
	glyph = actionPromptFrame->addImage(SDL_Rect{ 0, 0, mainHand->getSize().w, glyphSize },
		0xFFFFFFFF, "images/system/white.png", "action offhand glyph");
	glyph->ontop = true;
	auto offHandText = actionPromptFrame->addField("action offhand text", 64);
	offHandText->setFont(promptFont);
	offHandText->setText("Sneak");
	offHandText->setHJustify(Field::justify_t::CENTER);

	auto magic = actionPromptFrame->addFrame("action magic");
	magic->setSize(SDL_Rect{ 480, 400, maxWidth, promptHeight });
	magic->addImage(iconBackingPos,
		iconBackingColor, actionPromptBackingIconPath00.c_str(), "action img backing");
	magic->addImage(iconPos,
		iconColor, "images/system/white.png", "action img");
	glyph = actionPromptFrame->addImage(SDL_Rect{ 0, 0, mainHand->getSize().w, glyphSize },
		0xFFFFFFFF, "images/system/white.png", "action magic glyph");
	glyph->ontop = true;
	auto magicText = actionPromptFrame->addField("action magic text", 64);
	magicText->setFont(promptFont);
	magicText->setText("Cast spell");
	magicText->setHJustify(Field::justify_t::CENTER);

	auto sneak = actionPromptFrame->addFrame("action sneak");
	sneak->setSize(SDL_Rect{ 480, 400, maxWidth, promptHeight });
	sneak->addImage(iconBackingPos,
		iconBackingColor, actionPromptBackingIconPath00.c_str(), "action img backing");
	sneak->addImage(iconPos,
		iconColor, "images/system/white.png", "action img");
	glyph = actionPromptFrame->addImage(SDL_Rect{ 0, 0, mainHand->getSize().w, glyphSize },
		0xFFFFFFFF, "images/system/white.png", "action sneak glyph");
	glyph->ontop = true;
	auto sneakText = actionPromptFrame->addField("action sneak text", 64);
	sneakText->setFont(promptFont);
	sneakText->setText("Sneak");
	sneakText->setHJustify(Field::justify_t::CENTER);
}

int Player::HUD_t::actionPromptOffsetX = 280;
int Player::HUD_t::actionPromptOffsetY = 22;
int Player::HUD_t::actionPromptBackingSize = 44;
int Player::HUD_t::actionPromptIconSize = 32;
int Player::HUD_t::actionPromptIconOpacity = 255;
int Player::HUD_t::actionPromptIconBackingOpacity = 255;
void Player::HUD_t::updateActionPrompts()
{
	if ( !hudFrame )
	{
		return;
	}

	if ( !actionPromptsFrame )
	{
		createActionPrompts(player.playernum);
		if ( !actionPromptsFrame )
		{
			return;
		}
	}

	static ConsoleVariable<bool> disableActionPrompts(
	    "/disableprompts", false, "Disable action prompts in HUD");

	int playercount = 0;
	for (int c = 0; c < MAXPLAYERS; ++c) {
	    if (!client_disconnected[c] && players[c]->isLocalPlayer()) {
	        ++playercount;
	    }
	}

	if ( !bShowActionPrompts || playercount > 2 ||
	    (playercount == 2 && MainMenu::vertical_splitscreen) || *disableActionPrompts )
	{
		actionPromptsFrame->setDisabled(true);
		return;
	}
	actionPromptsFrame->setDisabled(false);
	actionPromptsFrame->setSize(SDL_Rect{ 0, 0, hudFrame->getSize().w, hudFrame->getSize().h });

	const int iconSize = Player::HUD_t::actionPromptIconSize;
	const int glyphSize = 32;
	const int iconBackingSize = Player::HUD_t::actionPromptBackingSize;
	const int maxWidth = std::max(iconSize, iconBackingSize);
	const int maxHeight = std::max(iconSize, iconBackingSize);
	const int promptHeight = maxHeight + glyphSize; // vertical space for prompts

	SDL_Rect iconPos{ maxWidth / 2 - iconSize / 2 - 0,
		maxHeight / 2 - iconSize / 2 - 0,
		iconSize,
		iconSize };
	SDL_Rect iconBackingPos{ maxWidth / 2 - iconBackingSize / 2,
		maxHeight / 2 - iconBackingSize / 2,
		iconBackingSize,
		iconBackingSize };

	struct PromptInfo {
		std::string name;
		ActionPrompts promptType;
		std::string inputName;
	};

	std::vector<PromptInfo> allPrompts;
	std::string blockBinding = Input::inputs[player.playernum].binding("Block");
	std::string sneakBinding = Input::inputs[player.playernum].binding("Sneak");
	const bool sneakingSeparateFromBlock = false;// blockBinding != sneakBinding;
	if ( sneakingSeparateFromBlock )
	{
		allPrompts.emplace_back(PromptInfo{ "action offhand", ACTION_PROMPT_OFFHAND, "Block" });
		allPrompts.emplace_back(PromptInfo{ "action sneak", ACTION_PROMPT_SNEAK, "Sneak" });
		allPrompts.emplace_back(PromptInfo{ "action magic", ACTION_PROMPT_MAGIC, "Cast Spell" });
		allPrompts.emplace_back(PromptInfo{ "action mainhand", ACTION_PROMPT_MAINHAND, "Attack" });
	}
	else
	{
		allPrompts.emplace_back(PromptInfo{ "action offhand", ACTION_PROMPT_OFFHAND, "Block" });
		allPrompts.emplace_back(PromptInfo{ "action sneak", ACTION_PROMPT_SNEAK, "Sneak" });
		allPrompts.emplace_back(PromptInfo{ "action magic", ACTION_PROMPT_MAGIC, "Cast Spell" });
		allPrompts.emplace_back(PromptInfo{ "action mainhand", ACTION_PROMPT_MAINHAND, "Attack" });
	}

	Uint32 iconBackingColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconBackingOpacity);
	Uint32 iconColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconOpacity);
	Uint32 iconFadedColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconOpacity * .5);
	Uint32 iconFadedBackingColor = makeColor(255, 255, 255, Player::HUD_t::actionPromptIconBackingOpacity * .5);

	int index = 0;
	for ( auto& promptInfo : allPrompts )
	{
		if ( auto prompt = actionPromptsFrame->findFrame(promptInfo.name.c_str()) )
		{
			prompt->setDisabled(false);

			std::string glyphName = promptInfo.name + " glyph";
			auto img = prompt->findImage("action img");
			auto glyph = actionPromptsFrame->findImage(glyphName.c_str());
			auto imgBacking = prompt->findImage("action img backing");

			SDL_Rect promptPos = prompt->getSize();
			promptPos.w = maxWidth;
			promptPos.h = promptHeight;
			int actionPromptOffsetXTotal = actionPromptOffsetX;
			if ( !player.hotbar.useHotbarFaceMenu )
			{
				actionPromptOffsetXTotal += 22;
			}
			switch ( index )
			{
				case 0: // to the left of hotbar
					promptPos.x = hudFrame->getSize().w / 2 - actionPromptOffsetXTotal;
					break;
				case 1: // to the left after the previous
					promptPos.x = hudFrame->getSize().w / 2 - actionPromptOffsetXTotal;
					promptPos.x -= 16;
					promptPos.x -= promptPos.w;
					break;
				case 2: // to the right of hotbar
					promptPos.x = hudFrame->getSize().w / 2 + actionPromptOffsetXTotal;
					promptPos.x -= promptPos.w;
					break;
				case 3: // to the right after the previous
					promptPos.x = hudFrame->getSize().w / 2 + actionPromptOffsetXTotal;
					promptPos.x += 16;
					break;
				default:
					break;
			}
			promptPos.y = hudFrame->getSize().h + player.hotbar.getHotbarStartY2() - promptPos.h;
			promptPos.y += maxHeight / 2;
			promptPos.y += actionPromptOffsetY;
			prompt->setSize(promptPos);

			img->pos = iconPos;
			imgBacking->pos = iconBackingPos;
			img->color = iconColor;
			imgBacking->color = iconBackingColor;

			Field* promptText = nullptr;
			switch ( promptInfo.promptType )
			{
				case ACTION_PROMPT_MAINHAND:
					promptText = actionPromptsFrame->findField("action mainhand text");
					break;
				case ACTION_PROMPT_OFFHAND:
					promptText = actionPromptsFrame->findField("action offhand text");
					break;
				case ACTION_PROMPT_MAGIC:
					promptText = actionPromptsFrame->findField("action magic text");
					break;
				case ACTION_PROMPT_SNEAK:
					promptText = actionPromptsFrame->findField("action sneak text");
					break;
				default:
					break;
			}

			if ( promptInfo.promptType == ACTION_PROMPT_SNEAK && !sneakingSeparateFromBlock )
			{
				prompt->setDisabled(true);
				promptText->setDisabled(true);
				glyph->disabled = true;
				++index;
				continue;
			}

			std::string textForPrompt = "";
			int skillForPrompt = getActionIconForPlayer(promptInfo.promptType, textForPrompt);
			if ( promptInfo.promptType == ACTION_PROMPT_OFFHAND && sneakingSeparateFromBlock
				&& skillForPrompt == PRO_STEALTH )
			{
				// offhand wants to display sneak - redundant, hide this prompt
				prompt->setDisabled(true);
				promptText->setDisabled(true);
				glyph->disabled = true;
				++index;
				continue;
			}

			if ( player.shootmode || player.gui_mode == GUI_MODE_FOLLOWERMENU || player.gui_mode == GUI_MODE_SIGN )
			{
				promptText->setDisabled(true);
			}
			else
			{
				promptText->setDisabled(false);
				promptText->setText(textForPrompt.c_str());
				SDL_Rect textPos;
				auto textGetLongestLine = Text::get(promptText->getLongestLine().c_str(),
					promptText->getFont(),
					promptText->getTextColor(),
					promptText->getOutlineColor());
				textPos.w = textGetLongestLine->getWidth();
				textPos.h = promptText->getNumTextLines() * Font::get(promptText->getFont())->height();
				textPos.x = promptPos.x + promptPos.w / 2 - textPos.w / 2;
				textPos.y = promptPos.y + imgBacking->pos.y - textPos.h - 4;
				promptText->setSize(textPos);
			}

			std::string skillImg = "";
			if ( skillForPrompt >= 0 && skillForPrompt < NUMPROFICIENCIES )
			{
				for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
				{
					if ( skill.skillId == skillForPrompt )
					{
						if ( skillCapstoneUnlocked(player.playernum, skillForPrompt) )
						{
							skillImg = skill.skillIconPathLegend32px;
						}
						else
						{
							skillImg = skill.skillIconPath32px;
						}
						break;
					}
				}
				if ( stats[player.playernum]->PROFICIENCIES[skillForPrompt] >= SKILL_LEVEL_LEGENDARY )
				{
					imgBacking->path = actionPromptBackingIconPath100;
				}
				else if ( stats[player.playernum]->PROFICIENCIES[skillForPrompt] >= SKILL_LEVEL_EXPERT )
				{
					imgBacking->path = actionPromptBackingIconPath60;
				}
				else if ( stats[player.playernum]->PROFICIENCIES[skillForPrompt] >= SKILL_LEVEL_BASIC )
				{
					imgBacking->path = actionPromptBackingIconPath20;
				}
				else
				{
					imgBacking->path = actionPromptBackingIconPath00;
				}
			}
			if ( skillImg == "" )
			{
				prompt->setDisabled(true);
			}
			else
			{
				prompt->setDisabled(false);
				img->path = skillImg;
				if ( img->path[0] != '*' )
				{
					img->path = '*' + img->path;
				}
			}

			bool pressed = false;
			/*if ( skillForPrompt == PRO_SHIELD && stats[player.playernum]->defending )
			{
				pressed = true;
			}
			if ( skillForPrompt == PRO_STEALTH && !stats[player.playernum]->defending
				&& stats[player.playernum]->sneaking )
			{
				pressed = true;
			}*/

			if ( skillForPrompt == PRO_STEALTH && promptInfo.promptType == ACTION_PROMPT_OFFHAND )
			{
				glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("Sneak", pressed);
			}
			else
			{
				glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding(promptInfo.inputName.c_str(), pressed);
			}
			glyph->disabled = prompt->isDisabled();
			if ( !player.shootmode || player.gui_mode == GUI_MODE_FOLLOWERMENU || player.gui_mode == GUI_MODE_SIGN )
			{
				glyph->disabled = true;
			}
			else if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				glyph->disabled = false;
			}
			else
			{
				glyph->disabled = false;
			}
			if ( auto imgGet = Image::get(glyph->path.c_str()) )
			{
				glyph->pos.w = (int)imgGet->getWidth();
				glyph->pos.h = (int)imgGet->getHeight();
			}
			glyph->pos.x = prompt->getSize().x + prompt->getSize().w / 2 - glyph->pos.w / 2; // center the x for the glyph
			const int glyphToImgPadY = 0;
			int pressedOffset = 0;
			/*if ( pressed )
			{
				if ( auto imgGet = Image::get(Input::inputs[player.playernum].getGlyphPathForBinding(promptInfo.inputName.c_str()).c_str()) )
				{
					int unpressedHeight = (int)imgGet->getHeight();
					if ( glyph->pos.h != unpressedHeight )
					{
						pressedOffset = (unpressedHeight - glyph->pos.h);
					}
				}
			}*/
			glyph->pos.y = prompt->getSize().y + img->pos.y + img->pos.h - glyphToImgPadY + pressedOffset; // just above the skill img with some padding

			if ( promptInfo.promptType == ACTION_PROMPT_SNEAK )
			{
				if ( !(!stats[player.playernum]->defending && stats[player.playernum]->sneaking) )
				{
					//glyph->disabled = true;
					img->color = iconFadedColor;
					//imgBacking->color = iconFadedBackingColor;
				}
			}
			if ( promptInfo.promptType == ACTION_PROMPT_OFFHAND && skillForPrompt == PRO_SHIELD
				&& sneakingSeparateFromBlock )
			{
				if ( !stats[player.playernum]->defending )
				{
					//glyph->disabled = true;
					img->color = iconFadedColor;
					//imgBacking->color = iconFadedBackingColor;
				}
			}
		}
		++index;
	}
}

static Frame* createMinimap(int player);

void createGameTimerFrame(const int player)
{
	auto& hud_t = players[player]->hud;
	hud_t.gameTimerFrame = hud_t.hudFrame->addFrame("timer");
	hud_t.gameTimerFrame->setHollow(true);
	hud_t.gameTimerFrame->setDisabled(true);
	hud_t.gameTimerFrame->setSize(SDL_Rect{ 0, 0, 142, 24 });

	auto txt = hud_t.gameTimerFrame->addField("timer txt", 64);
	txt->setText("00:00:00");
	txt->setSize(SDL_Rect{ 0, 0, hud_t.gameTimerFrame->getSize().w, hud_t.gameTimerFrame->getSize().h });
	txt->setFont("fonts/pixel_maz.ttf#32#2");
	txt->setVJustify(Field::justify_t::TOP);
	txt->setHJustify(Field::justify_t::LEFT);
	txt->setColor(makeColor(255, 255, 255, 255));
}

static void checkControllerState(int player) {
    auto& controllerFrame = players[player]->hud.controllerFrame;
    assert(controllerFrame);
    if (inputs.getPlayerIDAllowedKeyboard() != player) {
        auto controller = inputs.getController(player);
        if (!controller || (controller && !controller->isActive())) {
            if (controllerFrame->isHollow()) {
                controllerFrame->setHollow(false);
                controllerFrame->setDisabled(false);
                MainMenu::controllerDisconnected(player);
            }
            return;
        }
    }
    controllerFrame->setDisabled(true);
    controllerFrame->setHollow(true);
}

void Player::HUD_t::processHUD()
{
	const SDL_Rect hudSize{
	    players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight()};

	if ( !hudFrame )
	{
	    char name[32];
	    snprintf(name, sizeof(name), "player hud %d", player.playernum);
		hudFrame = gameUIFrame[player.playernum]->addFrame(name);
		hudFrame->setHollow(true);
		hudFrame->setBorder(0);
		hudFrame->setOwner(player.playernum);
	}

    if ( !controllerFrame )
    {
        controllerFrame = gameUIFrame[player.playernum]->addFrame("reconnect_controller");
        controllerFrame->setColor(0);
        controllerFrame->setOwner(player.playernum);
        controllerFrame->setBorder(0);
        controllerFrame->setDisabled(true);
        controllerFrame->setHollow(true);
        controllerFrame->setTickCallback([](Widget& widget){
            const int player = widget.getOwner();
            const bool disabled = ticks % TICKS_PER_SECOND >= TICKS_PER_SECOND / 2;
	        const SDL_Rect hudSize{
	            players[player]->camera_virtualx1(),
		        players[player]->camera_virtualy1(),
		        players[player]->camera_virtualWidth(),
		        players[player]->camera_virtualHeight()};
            auto frame = static_cast<Frame*>(&widget); assert(frame);
            auto image = frame->findImage("controller"); assert(image);
            image->pos.x = (hudSize.w - image->pos.w) / 2;
            image->pos.y = (hudSize.h - image->pos.h) / 2;
            image->disabled = disabled;
            auto field = frame->findField("label"); assert(field);
            field->setSize(image->pos);
            field->setInvisible(disabled);
            });

        auto dimmer = controllerFrame->addImage(
            SDL_Rect{0, 0, hudSize.w, hudSize.h},
            makeColor(0, 0, 0, 191),
            "#*images/system/white.png",
            "dimmer");

        const char* path = Input::getControllerGlyph();
        auto image = Image::get(path);
        const int w = image->getWidth();
        const int h = image->getHeight();
        const int x = (hudSize.w - w) / 2;
        const int y = (hudSize.h - h) / 2;
        auto controller = controllerFrame->addImage(
            SDL_Rect{x, y, w, h},
            0xffffffff, path,
            "controller");

        auto field = controllerFrame->addField("label", 16);
        field->setSize(SDL_Rect{x, y, w, h});
        field->setJustify(Field::justify_t::CENTER);
        field->setText((std::string("P") + std::to_string(player.playernum + 1)).c_str());
        field->setFont(bigfont_outline);
        switch (player.playernum) {
        default: field->setColor(uint32ColorPlayerX); break;
        case 0: field->setColor(uint32ColorPlayer1); break;
		case 1: field->setColor(uint32ColorPlayer2); break;
		case 2: field->setColor(uint32ColorPlayer3); break;
		case 3: field->setColor(uint32ColorPlayer4); break;
        }
    }

    controllerFrame->setSize(hudSize);
	hudFrame->setSize(hudSize);

	if ( gamePaused || nohud || !players[player.playernum]->isLocalPlayer() )
	{
		// hide
		hudFrame->setDisabled(true);
		controllerFrame->setDisabled(true);
	}
	else
	{
		hudFrame->setDisabled(false);
		if ( !controllerFrame->isHollow() )
		{
		    controllerFrame->setDisabled(false);
		}
	}

    if ( MainMenu::isPlayerSignedIn(player.playernum) && players[player.playernum]->isLocalPlayer() )
    {
	    checkControllerState(player.playernum);
	}

    if ( !minimapFrame )
    {
        minimapFrame = createMinimap(player.playernum);
    }
	if ( !gameTimerFrame )
	{
		createGameTimerFrame(player.playernum);
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
	if ( !StatusEffectQueue[player.playernum].statusEffectFrame )
	{
		createStatusEffectQueue(player.playernum);
	}

	updateGameTimer();
	updateXPBar();
	updateHPBar();
	updateMPBar();
	updateActionPrompts();
	updateUINavigation();
	enemyHPDamageBarHandler[player.playernum].cullExpiredHPBars();
	enemyBarFrame->setDisabled(true);
	enemyBarFrameHUD->setDisabled(true);
	for ( auto& HPBar : enemyHPDamageBarHandler[player.playernum].HPBars )
	{
		updateEnemyBar2(enemyBarFrame, &HPBar.second);
	}
	updateStatusEffectQueue(player.playernum);
}

void Player::MessageZone_t::createChatbox()
{
	char name[32];
	snprintf(name, sizeof(name), "player chat %d", player.playernum);
	if ( !gameUIFrame[player.playernum]->findFrame(name) )
	{
		Frame* chatMainFrame = gameUIFrame[player.playernum]->addFrame(name);
		chatMainFrame->setHollow(true);
		chatMainFrame->setBorder(0);
		chatMainFrame->setOwner(player.playernum);
		chatMainFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
			players[player.playernum]->camera_virtualy1(),
			players[player.playernum]->camera_virtualWidth(),
			players[player.playernum]->camera_virtualHeight() });
		chatFrame = chatMainFrame;
		Frame* messages = chatMainFrame->addFrame("message box");
		messages->setHollow(true);

		static const char* bigfont = "fonts/pixelmix.ttf#16#2";
		SDL_Rect entryPos{ 0, 0, messages->getSize().w, messages->getSize().h };
		for ( int i = 0; i < MESSAGE_MAX_ENTRIES; ++i )
		{
			char msgName[32];
			snprintf(msgName, sizeof(msgName), "message %d", i);
			auto entry = messages->addField(msgName, ADD_MESSAGE_BUFFER_LENGTH);
			entry->setFont(bigfont);
			entry->setSize(entryPos);
			entry->setDisabled(true);
			entry->setVJustify(Field::justify_t::TOP);
		}
	}
}

void Player::MessageZone_t::processChatbox()
{
	if (!chatFrame) {
		createChatbox();
	}

	chatFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	int playercount = 0;
	for (int c = 0; c < MAXPLAYERS; ++c) {
	    if (!client_disconnected[c] && players[c]->isLocalPlayer()) {
	        ++playercount;
	    }
	}

	Frame* messageBoxFrame = chatFrame->findFrame("message box");
	messageBoxFrame->setDisabled(gamePaused || (!player.shootmode && playercount > 2));
	if (messageBoxFrame->isDisabled()) {
		return;
	}

	static const char* bigfont = "fonts/pixelmix.ttf#16#2";
	static const char* smallfont = "fonts/pixel_maz_multiline.ttf#16#2";
    bool useBigFont = playercount == 1 || (playercount == 2 && !MainMenu::vertical_splitscreen);

    bool pushPaddingX = !players[player.playernum]->shootmode &&
        ((playercount == 1 && stats[player.playernum]->cloak && stats[player.playernum]->cloak->type == CLOAK_BACKPACK) ||
        (playercount == 2 && !MainMenu::vertical_splitscreen));

	const int leftAlignedPaddingX = pushPaddingX ? 240 : 8;
	const int leftAlignedBottomY = 200;
	const int topAlignedPaddingX = 8;
	const int topAlignedPaddingY = playercount > 2 ? 32 : 8;
	SDL_Rect messageboxTopAlignedPos{
	    topAlignedPaddingX,
	    topAlignedPaddingY,
		players[player.playernum]->camera_virtualWidth() - topAlignedPaddingX,
		players[player.playernum]->camera_virtualHeight() - topAlignedPaddingY };
	SDL_Rect messageboxLeftAlignedPos{
	    leftAlignedPaddingX,
	    0,
	    players[player.playernum]->camera_virtualWidth() - leftAlignedPaddingX,
		players[player.playernum]->camera_virtualHeight() - leftAlignedBottomY };

    static ConsoleVariable<bool> cvar_top_aligned("/topmessages", false);
	const bool useLeftAligned = (!(*cvar_top_aligned) || !player.shootmode) && playercount <= 2;

	SDL_Rect messageBoxSize = useLeftAligned ?
	    messageboxLeftAlignedPos : messageboxTopAlignedPos;

	static ConsoleVariable<std::string> alignment("/alignmessages", "left");

	for ( int i = 0; i < MESSAGE_MAX_ENTRIES; ++i )
	{
	    char msgName[32];
		snprintf(msgName, sizeof(msgName), "message %d", i);
		if ( auto entry = messageBoxFrame->findField(msgName) )
		{
			entry->setDisabled(true);

			// set alignment
			if (*alignment == "left") {
				entry->setHJustify(Field::justify_t::LEFT);
			} else if (*alignment == "center") {
				entry->setHJustify(Field::justify_t::CENTER);
			} else if (*alignment == "right") {
			    entry->setHJustify(Field::justify_t::RIGHT);
			} else {
				entry->setHJustify(Field::justify_t::LEFT);
			}
		}
	}

	const bool messageDrawDescending = !useLeftAligned;
	const int entryPaddingY = playercount > 2 ? 0 : 4;

	int currentY = messageDrawDescending ?
	    0 : messageBoxSize.h;

	int index = 0;

	auto it = notification_messages.begin();
	auto end = notification_messages.end();
	auto rit = notification_messages.rbegin();
	auto rend = notification_messages.rend();
	for ( ; messageDrawDescending ? rit != rend : it != end; ++it, ++rit )
	{
	    Message* current = messageDrawDescending ? *rit : *it;
		if (index >= MESSAGE_MAX_ENTRIES)
		{
			break;
		}

		Uint32 color = (current->text->color & 0x00ffffff) | ((Uint32)current->alpha << 24);

	    char msgName[32];
		snprintf(msgName, sizeof(msgName), "message %d", index);
		if ( auto entry = messageBoxFrame->findField(msgName) )
		{
			entry->setDisabled(false);
			entry->setColor(color);
			entry->setText(current->text->data);
			entry->setFont(useBigFont ? bigfont : smallfont);

            Font* fontGet = Font::get(entry->getFont());
			Text* textGet = Text::get(entry->getText(), entry->getFont(),
				makeColor(255, 255, 255, 255), makeColor(0, 0, 0, 255));

			int w = textGet->getWidth();
			int h = textGet->getHeight() * current->text->lines;

			if (!messageDrawDescending) {
				currentY -= h;
				currentY -= entryPaddingY;
			}

			SDL_Rect pos = entry->getSize();
			pos.x = 0;
			pos.y = currentY;
			pos.w = messageBoxFrame->getSize().w;
			pos.h = h;
			entry->setSize(pos);

			if (messageDrawDescending) {
				currentY += h;
			    currentY += entryPaddingY;
			}
		}
		++index;
	}

	messageBoxFrame->setSize(messageBoxSize);
}

ConsoleVariable<bool> shareMinimap("/shareminimap", true);
Frame* minimapFrame = nullptr; // shared minimap

void doSharedMinimap() {
    if (!minimapFrame) {
		minimapFrame = gui->addFrame("shared_minimap");
		minimapFrame->setColor(0);
		minimapFrame->setHollow(true);
		minimapFrame->setDrawCallback([](const Widget& widget, SDL_Rect rect){
            drawMinimap(widget.getOwner(), rect);
            });
		minimapFrame->setTickCallback([](Widget& widget){
	        int playercount = 0;
	        for (int c = 0; c < MAXPLAYERS; ++c) {
		        if (!client_disconnected[c] && players[c]->isLocalPlayer()) {
			        ++playercount;
		        }
	        }
            if (intro || MainMenu::isCutsceneActive() || playercount < 3 || !*shareMinimap) {
				minimapFrame->setInvisible(true);
            } else {
				minimapFrame->setInvisible(false);
            }
            if (playercount == 3) {
                const int size = std::min(Frame::virtualScreenX / 2, Frame::virtualScreenY / 2);
				minimapFrame->setSize(SDL_Rect{
                    (Frame::virtualScreenX + ((Frame::virtualScreenX / 2) - size)) / 2,
                    (Frame::virtualScreenY + ((Frame::virtualScreenY / 2) - size)) / 2,
                    size, size});
            } else if (playercount == 4) {
                constexpr int size = 128;
				minimapFrame->setSize(SDL_Rect{
                    (Frame::virtualScreenX - size) / 2,
                    Frame::virtualScreenY / 2,
                    size, size});
            }
            });
    }
	minimapFrame->setOwner(clientnum);
}

static Frame* createMinimap(int player) {
    std::string name = "minimap";
    name.append(std::to_string(player));
    auto& minimap = players[player]->minimap;
    minimap.real_scale = minimapScale;
    minimap.scale = minimapScale;
    Frame* parent = players[player]->hud.hudFrame;
    Frame* window = parent->addFrame(name.c_str());
    window->setSize(SDL_Rect{0, 0, 0, 0});
    window->setColor(0);
    window->setOwner(player);

    window->setDrawCallback([](const Widget& widget, SDL_Rect rect){
        drawMinimap(widget.getOwner(), rect);
        });

    window->setTickCallback([](Widget& widget){
        int playercount = 0;
	    for (int c = 0; c < MAXPLAYERS; ++c) {
	        if (!client_disconnected[c] && players[c]->isLocalPlayer()) {
	            ++playercount;
	        }
	    }

	    widget.setInvisible(*shareMinimap && playercount > 2);

        bool reducedSize = playercount > 2 || (playercount == 2 && MainMenu::vertical_splitscreen);

        auto player = widget.getOwner();
        auto& minimap = players[player]->minimap;
        auto& input = Input::inputs[player];
        if ( !gamePaused && players[player]->bControlEnabled 
			&& !players[player]->usingCommand() && players[player]->shootmode && input.consumeBinaryToggle("Minimap Scale")) {
            if (minimap.real_scale > 75.0) {
                minimap.real_scale = 75.0;
            }
            else if (minimap.real_scale > 50.0) {
                minimap.real_scale = 50.0;
            }
            else if (minimap.real_scale > 25.0) {
                minimap.real_scale = 25.0;
            }
            else {
                minimap.real_scale = reducedSize ? 75.0 :  100.0;
            }
        }

        auto& scale_ang = minimap.scale_ang;
        auto& scale = minimap.scale;
        if (minimap.big) {
            if (scale_ang < PI / 2.0) {
                scale_ang += (PI / fpsLimit) * 2.0;
                if (scale_ang > PI / 2.0) {
                    scale_ang = PI / 2.0;
                }
            }
        } else {
            if (scale_ang > 0.0) {
                scale_ang -= (PI / fpsLimit) * 2.0;
                if (scale_ang < 0.0) {
                    scale_ang = 0.0;
                }
            }
        }

        real_t factor0 = 1.0 - sin(scale_ang);
        real_t factor1 = sin(scale_ang);
        real_t scale_small = std::min(reducedSize ? 25.0 : 50.0, minimap.real_scale);
        real_t scale_big = std::min(reducedSize ? 75.0 : 100.0, minimap.real_scale);

        scale = factor0 * scale_small + factor1 * scale_big;

        Frame* parent = players[player]->hud.hudFrame;

        int x = factor0 * (parent->getSize().w - scale * 4) +
            factor1 * (parent->getSize().w - scale * 4) / 2;
        int y = factor0 * (parent->getSize().h - scale * 4) +
            factor1 * (parent->getSize().h - scale * 4) / 2;

        auto frame = static_cast<Frame*>(&widget);
        frame->setSize(SDL_Rect{x, y, (int)(minimap.scale * 4), (int)(minimap.scale * 4)});
		if ( frame->isInvisible() )
		{
			frame->setHollow(true);
		}
		else
		{
			frame->setHollow(false);
		}
        });

    return window;
}

void openMinimap(int player) {
    Frame* minimap = players[player]->hud.minimapFrame;
    if (minimap) {
        players[player]->minimap.big = (players[player]->minimap.big==false);
    }
}

void openMapWindow(int player) {
    auto& frame = players[player]->hud.mapWindow;
    if (frame) {
        frame->removeSelf();
        frame = nullptr;
		if ( players[player]->gui_mode == GUI_MODE_NONE ) {
			players[player]->shootmode = true;
		}
		else
		{
			players[player]->GUI.returnToPreviousActiveModule();
		}
        return;
    }

	players[player]->GUI.previousModule = players[player]->GUI.activeModule;
	if ( players[player]->shootmode )
	{
		players[player]->openStatusScreen(GUI_MODE_NONE,
			INVENTORY_MODE_ITEM, Player::GUI_t::MODULE_MAP);
	}
	else
	{
		players[player]->openStatusScreen(GUI_MODE_INVENTORY,
			players[player]->inventory_mode, Player::GUI_t::MODULE_MAP); // Reset the GUI to the inventory.
	}

    auto& otherWindow = players[player]->hud.logWindow;
    if (otherWindow) {
        otherWindow->removeSelf();
        otherWindow = nullptr;
    }
    Frame* parent = players[player]->hud.hudFrame;
    const SDL_Rect size = parent->getSize();
    const int _w = std::max(Frame::virtualScreenX / 2, size.w - 432);
    const int _h = std::max(Frame::virtualScreenY / 2, size.h - 256);
    const int w = std::min(_w, _h);
    const int h = std::min(_w, _h);
    frame = parent->addFrame("minimap_window");
    frame->setOwner(player);
    frame->setSize(SDL_Rect{(size.w - w) / 2, (size.h - h) / 2 - 32, w, h});
    frame->setBorderColor(makeColor(51, 33, 26, 255));
    frame->setColor(0);
    //frame->setBorder(2);
    frame->setBorder(0);
    frame->setColor(makeColor(mapBgColor->x, mapBgColor->y, mapBgColor->z, mapBgColor->w));
    frame->setTickCallback([](Widget& widget){
        const int player = widget.getOwner();
        auto frame = static_cast<Frame*>(&widget);
        if (players[player]->shootmode) {
            players[player]->hud.mapWindow = nullptr;
            frame->removeSelf();
        }
        });

    // frame images
    {
        frame->addImage(
            SDL_Rect{0, 0, 16, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_TL00.png",
            "TL");
        frame->addImage(
            SDL_Rect{16, 0, w - 32, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_T00.png",
            "T");
        frame->addImage(
            SDL_Rect{w - 16, 0, 16, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_TR00.png",
            "TR");
        auto L = frame->addImage(
            SDL_Rect{0, 32, 4, h - 48},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_L00.png",
            "L");
        L->ontop = true;
        auto R = frame->addImage(
            SDL_Rect{w - 4, 32, 4, h - 48},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_R00.png",
            "R");
        R->ontop = true;
        auto BL = frame->addImage(
            SDL_Rect{0, h - 16, 16, 16},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_BL00.png",
            "BL");
        BL->ontop = true;
        auto B = frame->addImage(
            SDL_Rect{16, h - 4, w - 32, 4},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_B00.png",
            "B");
        B->ontop = true;
        auto BR = frame->addImage(
            SDL_Rect{w - 16, h - 16, 16, 16},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_BR00.png",
            "BR");
        BR->ontop = true;
    }

    auto container = frame->addFrame("container");
	container->setSize(SDL_Rect{0, 32, w, h - 32});
    container->setBorderColor(makeColor(51, 33, 26, 255));
	//container->setBorder(2);
	container->setBorder(0);
	container->setColor(0);

    const int map_size = std::min(w - 32, h - 64);
	auto minimap = container->addFrame("minimap");
	minimap->setSize(SDL_Rect{(w - map_size) / 2, (h - 32 - map_size) / 2, map_size, map_size});
	minimap->setColor(0);
    minimap->setBorder(0);

    struct Position {
        real_t x;
        real_t y;
    };
	static Position minimap_cursor[MAXPLAYERS];
	minimap_cursor[player].x = map_size / 2;
	minimap_cursor[player].y = map_size / 2;

	if ( !inputs.getVirtualMouse(player)->draw_cursor )
	{
		Input::inputs[player].consumeBinary("MinimapPing");
	}

	minimap->setDrawCallback([](const Widget& widget, SDL_Rect rect){
	    int player = widget.getOwner();
        drawMinimap(player, rect);
        if (!inputs.getVirtualMouse(player)->draw_cursor) {
	        auto& cursor = minimap_cursor[player];
			auto image = Image::get("*#images/ui/MapAndLog/cursor.png");
			SDL_Rect pos;
			pos.x = cursor.x - image->getWidth() / 2 + rect.x;
			pos.y = cursor.y - image->getHeight() / 2 + rect.y;
			pos.w = image->getWidth();
			pos.h = image->getHeight();
			image->drawColor(nullptr, pos, SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY}, 0xffffffff);
        }
        });

	minimap->setTickCallback([](Widget& widget){
	    int player = widget.getOwner();
	    auto& input = Input::inputs[player];
	    auto minimap = static_cast<Frame*>(&widget);

	    static ConsoleVariable<float> speed("/minimap_cursor_speed", 2.f);

	    // gamepad moves cursor with right stick
	    auto& cursor = minimap_cursor[player];
        cursor.x += (input.analog("MinimapRight") - input.analog("MinimapLeft")) * (*speed);
        cursor.y += (input.analog("MinimapDown") - input.analog("MinimapUp")) * (*speed);
        cursor.x = std::min(std::max((real_t)0, cursor.x), (real_t)minimap->getSize().w);
        cursor.y = std::min(std::max((real_t)0, cursor.y), (real_t)minimap->getSize().h);
        input.consumeBindingsSharedWithBinding("MinimapRight");
        input.consumeBindingsSharedWithBinding("MinimapLeft");
        input.consumeBindingsSharedWithBinding("MinimapDown");
        input.consumeBindingsSharedWithBinding("MinimapUp");
        if (input.consumeBinaryToggle("MinimapClose")) {
			input.consumeBindingsSharedWithBinding("MinimapClose");
            if (players[player]->hud.mapWindow) {
                players[player]->hud.mapWindow->removeSelf();
                players[player]->hud.mapWindow = nullptr;
                if (players[player]->gui_mode == GUI_MODE_NONE) {
                    players[player]->shootmode = true;
                }
				else
				{
					players[player]->GUI.returnToPreviousActiveModule();
				}
            }
        }

        static ConsoleVariable<bool> minimapGimpEnabled("/minimap_gimp_enabled", true);

        // minimap pings
		if (minimapPingGimpTimer[player] <= 0 || !*minimapGimpEnabled) {
            if (input.consumeBinaryToggle("MinimapPing")) {
                auto mouse_position = inputs.getVirtualMouse(player)->draw_cursor ?
                    minimap->getRelativeMousePosition(false):
                    SDL_Rect{(int)cursor.x, (int)cursor.y, minimap->getSize().w, minimap->getSize().h};
                messagePlayer(clientnum, MESSAGE_DEBUG, "[Minimap] Clicked %d %d %d %d",
                    mouse_position.x, mouse_position.y, mouse_position.w, mouse_position.h);
                if (mouse_position.w > 0 && mouse_position.h > 0) {
                    const int size = std::max((int)map.width, (int)map.height);
                    const int xdiff = std::max(0, (int)map.height - (int)map.width) / 2;
                    const int ydiff = std::max(0, (int)map.width - (int)map.height) / 2;
                    const int x = (mouse_position.x * size) / mouse_position.w - xdiff;
                    const int y = (mouse_position.y * size) / mouse_position.h - ydiff;
                    if (x >= 0 && y >= 0 && x < map.width && y < map.height) {
                        MinimapPing newPing(ticks, player, x, y);
		                if ( multiplayer != CLIENT ) {
			                minimapPingAdd(player, player, newPing);
		                }
		                sendMinimapPing(player, newPing.x, newPing.y);

                        // can also issue move commands via minimap
                        FollowerRadialMenu& followerMenu = FollowerMenu[player];
		                if ( !followerMenu.menuToggleClick && followerMenu.selectMoveTo )
		                {
			                if ( followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT )
			                {
                                if (!players[player]->usingCommand() && players[player]->bControlEnabled) {
				                    createParticleFollowerCommand(newPing.x, newPing.y, 0, 174);
				                    followerMenu.optionSelected = ALLY_CMD_MOVETO_CONFIRM;
				                    followerMenu.selectMoveTo = false;
				                    followerMenu.moveToX = static_cast<int>(newPing.x);
				                    followerMenu.moveToY = static_cast<int>(newPing.y);
			                    }
			                }
			            }
			        }
                }
            }
        } else { // if (minimapPingGimpTimer[player] <= 0)
		    --minimapPingGimpTimer[player];
        }
        });

    auto label = frame->addField("label", 64);
    label->setSize(SDL_Rect{16, 0, w - 40, 32});
    label->setHJustify(Field::justify_t::LEFT);
    label->setVJustify(Field::justify_t::CENTER);
    label->setFont(bigfont_outline);
    label->setText("Map");

    auto close_button = frame->addButton("close");
    close_button->setSize(SDL_Rect{frame->getSize().w - 30, 4, 26, 26});
	close_button->setColor(makeColor(255, 255, 255, 255));
	close_button->setHighlightColor(makeColor(255, 255, 255, 255));
	close_button->setText("X");
	close_button->setFont(smallfont_outline);
	close_button->setHideGlyphs(true);
	close_button->setHideKeyboardGlyphs(true);
	close_button->setHideSelectors(true);
	close_button->setMenuConfirmControlType(0);
	close_button->setBackground("*#images/ui/Shop/Button_X_00.png");
	close_button->setBackgroundHighlighted("*#images/ui/Shop/Button_XHigh_00.png");
	close_button->setBackgroundActivated("*#images/ui/Shop/Button_XPress_00.png");
	close_button->setTextHighlightColor(makeColor(201, 162, 100, 255));
    close_button->setCallback([](Button& button){
        const int player = button.getOwner();
        players[player]->hud.mapWindow = nullptr;
        auto parent = static_cast<Frame*>(button.getParent());
        parent->removeSelf();
        if (players[player]->gui_mode == GUI_MODE_NONE) {
            players[player]->shootmode = true;
        }
        });
}

static ConsoleCommand ccmd_log_clear("/log_clear", "Clears log history",
    [](int argc, const char** argv){
    list_FreeAll(&messages);
    });

void addMessageToLogWindow(int player, string_t* string) {
    auto& frame = players[player]->hud.logWindow;
    if (!frame || !string) {
        return;
    }

    const int w = frame->getSize().w;
    const int h = frame->getSize().h;

    auto subframe = frame->findFrame("subframe"); assert(subframe);
    auto subframe_size = subframe->getActualSize();
    int y = subframe_size.h;

    static ConsoleVariable<bool> timestamp_messages("/log_timestamp", true);

    char buf[1024];
    const Uint32 time = string->time / TICKS_PER_SECOND;
    const Uint32 hour = time / 3600;
    const Uint32 min = (time / 60) % 60;
    const Uint32 sec = time % 60;
    const int result = *timestamp_messages ?
        snprintf(buf, sizeof(buf), "[%.2u:%.2u:%.2u] %s",
            hour, min, sec, string->data):
        snprintf(buf, sizeof(buf), "%s", string->data);
    const int size = std::min(std::max(0, (int)sizeof(buf)), result);

    static ConsoleVariable<std::string> font("/log_font",
        "fonts/PixelMaz_monospace.ttf#32#2");

    auto field = subframe->addField("field", size + 1);
    auto text = Text::get(buf, font->c_str(),
        uint32ColorWhite, uint32ColorBlack);
    const int text_h = (int)text->getHeight() * (int)string->lines + 2;
    const int text_w = (int)text->getWidth();
    field->setSize(SDL_Rect{8, y, text_w, text_h});
    field->setFont(font->c_str());
    field->setColor(string->color);
    field->setText(buf);

    (void)snprintf(buf, sizeof(buf), "[%.2u:%.2u:%.2u]", hour, min, sec);
    field->setTooltip(buf);

    const int new_w = std::max(subframe_size.w, text_w + 8);

    y += text_h;
    if (subframe_size.y >= subframe_size.h - subframe->getSize().h) {
        // advance scroll because we're already at bottom
        const int limit = new_w > w ?
            y - subframe->getSize().h + 16:
            y - subframe->getSize().h;
        subframe->setActualSize(SDL_Rect{subframe_size.x,
            std::max(0, limit), new_w, y});
    } else {
        // retain scroll position because we're looking at past history
        subframe->setActualSize(SDL_Rect{
            subframe_size.x, subframe_size.y, new_w, y});
    }
}

void openLogWindow(int player) {
    auto& frame = players[player]->hud.logWindow;
    if (frame) {
        frame->removeSelf();
        frame = nullptr;
		if ( players[player]->gui_mode == GUI_MODE_NONE ) {
			players[player]->shootmode = true;
		}
		else
		{
			players[player]->GUI.returnToPreviousActiveModule();
		}
        return;
    }

	players[player]->GUI.previousModule = players[player]->GUI.activeModule;
	if ( players[player]->shootmode )
	{
		players[player]->openStatusScreen(GUI_MODE_NONE,
			INVENTORY_MODE_ITEM, Player::GUI_t::MODULE_LOG);
	}
	else
	{
		players[player]->openStatusScreen(GUI_MODE_INVENTORY,
			players[player]->inventory_mode, Player::GUI_t::MODULE_LOG); // Reset the GUI to the inventory.
	}

    auto& otherWindow = players[player]->hud.mapWindow;
    if (otherWindow) {
        otherWindow->removeSelf();
        otherWindow = nullptr;
    }
    Frame* parent = players[player]->hud.hudFrame;
    const SDL_Rect size = parent->getSize();
    const int w = std::max(Frame::virtualScreenX / 2, size.w - 432);
    const int h = std::max(Frame::virtualScreenY / 2, size.h - 256);

    frame = parent->addFrame("log_window");
    frame->setOwner(player);
    frame->setSize(SDL_Rect{(size.w - w) / 2, (size.h - h) / 2 - 32, w, h});
    frame->setBorderColor(makeColor(51, 33, 26, 255));
    frame->setColor(makeColor(logBgColor->x, logBgColor->y, logBgColor->z, logBgColor->w));
    //frame->setBorder(2);
    frame->setBorder(0);
    frame->setTickCallback([](Widget& widget){
        const int player = widget.getOwner();
        auto frame = static_cast<Frame*>(&widget);
        if (players[player]->shootmode) {
            players[player]->hud.logWindow = nullptr;
            frame->removeSelf();
        }

        const int w = frame->getSize().w;
        const int h = frame->getSize().h;

        static ConsoleVariable<int> glyph1_kb("/log_kb_glyph1", 32);
        static ConsoleVariable<int> glyph2_kb("/log_kb_glyph2", 92);
        static ConsoleVariable<int> glyph3_kb("/log_kb_glyph3", 330);
        static ConsoleVariable<int> glyph4_kb("/log_kb_glyph4", 396);
        static ConsoleVariable<int> glyph5_kb("/log_kb_glyph5", 660);
        static ConsoleVariable<int> glyph6_kb("/log_kb_glyph6", 694);

        static ConsoleVariable<int> glyph1_gamepad("/log_gamepad_glyph1", 32);
        static ConsoleVariable<int> glyph2_gamepad("/log_gamepad_glyph2", 92);
        static ConsoleVariable<int> glyph3_gamepad("/log_gamepad_glyph3", 330);
        static ConsoleVariable<int> glyph4_gamepad("/log_gamepad_glyph4", 396);
        static ConsoleVariable<int> glyph5_gamepad("/log_gamepad_glyph5", 660);
        static ConsoleVariable<int> glyph6_gamepad("/log_gamepad_glyph6", 694);

        const bool using_keyboard = Input::inputs[player].getPlayerControlType() ==
            Input::playerControlType_t::PLAYER_CONTROLLED_BY_KEYBOARD;

        struct Glyph {
            const char* name;
            int kb_x;
            int gamepad_x;
        };
        const Glyph glyphs[] = {
            {"LogHome", *glyph1_kb, *glyph1_gamepad},
            {"LogEnd", *glyph2_kb, *glyph2_gamepad},
            {"LogPageUp", *glyph3_kb, *glyph3_gamepad},
            {"LogPageDown", *glyph4_kb, *glyph4_gamepad},
            {"LogScrollUp", *glyph5_kb, *glyph5_gamepad},
            {"LogScrollDown", *glyph6_kb, *glyph6_gamepad},
        };
        const int num_glyphs = sizeof(glyphs) / sizeof(glyphs[0]);

        const bool pressed = (ticks % TICKS_PER_SECOND) >= TICKS_PER_SECOND / 2;

        for (int c = 0; c < num_glyphs; ++c) {
            auto path = Input::inputs[player].getGlyphPathForBinding(glyphs[c].name, pressed);
            auto glyph = Image::get(path.c_str()); assert(glyph);
            auto image = frame->findImage(glyphs[c].name); assert(image);
            image->color = 0xffffffff;
            image->path = path;
            image->pos = SDL_Rect{
                using_keyboard ?
                    (int)(glyphs[c].kb_x - glyph->getWidth() / 2):
                    (int)(glyphs[c].gamepad_x - glyph->getWidth() / 2),
                (int)(h - 16 - glyph->getHeight() / 2),
                (int)glyph->getWidth(),
                (int)glyph->getHeight()};
        }

        auto help_left = frame->findField("help_left"); assert(help_left);
        auto help_center = frame->findField("help_center"); assert(help_center);
        auto help_right = frame->findField("help_right"); assert(help_right);

        static ConsoleVariable<std::string> help_left_kb("/log_kb_help_left", "              /              Skip to start/end");
        static ConsoleVariable<std::string> help_center_kb("/log_kb_help_center", "              /              Page Up/Down");
        static ConsoleVariable<std::string> help_right_kb("/log_kb_help_right", "        /        Scroll Up/Down");
        static ConsoleVariable<std::string> help_left_gamepad("/log_gamepad_help_left", "             /              Skip to start/end");
        static ConsoleVariable<std::string> help_center_gamepad("/log_gamepad_help_center", "              /              Page Up/Down");
        static ConsoleVariable<std::string> help_right_gamepad("/log_gamepad_help_right", "        /        Scroll Up/Down");

        if (using_keyboard) {
            help_left->setText(help_left_kb->c_str());
            help_center->setText(help_center_kb->c_str());
            help_right->setText(help_right_kb->c_str());
        } else {
            help_left->setText(help_left_gamepad->c_str());
            help_center->setText(help_center_gamepad->c_str());
            help_right->setText(help_right_gamepad->c_str());
        }

        auto subframe = frame->findFrame("subframe"); assert(subframe);
        auto subframe_size = subframe->getActualSize();

        if (Input::inputs[player].consumeBinaryToggle("LogHome")) {
            Input::inputs[player].consumeBindingsSharedWithBinding("LogHome");
            subframe_size.x = 0;
            subframe_size.y = 0;
            subframe->setActualSize(subframe_size);
        }
        if (Input::inputs[player].consumeBinaryToggle("LogEnd")) {
            Input::inputs[player].consumeBindingsSharedWithBinding("LogEnd");
            const int limit = subframe_size.w > w ?
                subframe_size.h - subframe->getSize().h + 16:
                subframe_size.h - subframe->getSize().h;
            subframe_size.x = 0;
            subframe_size.y = std::max(0, limit);
            subframe->setActualSize(subframe_size);
        }
        if (Input::inputs[player].consumeBinaryToggle("LogPageUp")) {
            Input::inputs[player].consumeBindingsSharedWithBinding("LogPageUp");
            subframe_size.y -= subframe->getSize().h;
            subframe_size.y = std::max(0, subframe_size.y);
            subframe->setActualSize(subframe_size);
        }
        if (Input::inputs[player].consumeBinaryToggle("LogPageDown")) {
            Input::inputs[player].consumeBindingsSharedWithBinding("LogPageDown");
            subframe_size.y += subframe->getSize().h;
            const int limit = subframe_size.w > w ?
                subframe_size.h - subframe->getSize().h + 16:
                subframe_size.h - subframe->getSize().h;
            subframe_size.y = std::min(std::max(0, limit), subframe_size.y);
            subframe->setActualSize(subframe_size);
        }
		if ( Input::inputs[player].consumeBinaryToggle("LogClose") ) {
			Input::inputs[player].consumeBindingsSharedWithBinding("LogClose");
			if ( players[player]->hud.logWindow ) {
				players[player]->hud.logWindow->removeSelf();
				players[player]->hud.logWindow = nullptr;
				if ( players[player]->gui_mode == GUI_MODE_NONE ) {
					players[player]->shootmode = true;
				}
				else
				{
					players[player]->GUI.returnToPreviousActiveModule();
				}
			}
		}
        });

    // frame images
    {
        frame->addImage(
            SDL_Rect{0, 0, 16, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_TL00.png",
            "TL");
        frame->addImage(
            SDL_Rect{16, 0, w - 32, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_T00.png",
            "T");
        frame->addImage(
            SDL_Rect{w - 16, 0, 16, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_TR00.png",
            "TR");
        auto L = frame->addImage(
            SDL_Rect{0, 32, 4, h - 64},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_L00.png",
            "L");
        L->ontop = true;
        auto R = frame->addImage(
            SDL_Rect{w - 4, 32, 4, h - 64},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_R00.png",
            "R");
        R->ontop = true;
        frame->addImage(
            SDL_Rect{0, h - 32, 16, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_BL01.png",
            "BL");
        frame->addImage(
            SDL_Rect{16, h - 32, w - 32, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_B01.png",
            "B");
        frame->addImage(
            SDL_Rect{w - 16, h - 32, 16, 32},
            0xffffffff,
            "*#images/ui/MapAndLog/Hover_BR01.png",
            "BR");
    }

    auto subframe = frame->addFrame("subframe");
    subframe->setScrollWithLeftControls(false);
    subframe->setSize(SDL_Rect{0, 32, w, h - 64});
    subframe->setActualSize(SDL_Rect{0, 0, w, 4});
    subframe->setBorderColor(makeColor(22, 24, 29, 255));
    subframe->setSliderColor(makeColor(44, 48, 58, 255));
    subframe->setColor(makeColor(0, 0, 0, 0));
    subframe->setScrollBarsEnabled(true);
    //subframe->setBorder(2);
    subframe->setBorder(0);

    for (auto node = messages.first; node != nullptr; node = node->next) {
        auto string = (string_t*)node->element;
        addMessageToLogWindow(player, string);
    }

    auto label = frame->addField("label", 64);
    label->setSize(SDL_Rect{16, 0, w - 40, 32});
    label->setHJustify(Field::justify_t::LEFT);
    label->setVJustify(Field::justify_t::CENTER);
    label->setFont(bigfont_outline);
    label->setText("Log");

    auto close_button = frame->addButton("close");
    close_button->setSize(SDL_Rect{frame->getSize().w - 30, 4, 26, 26});
	close_button->setColor(makeColor(255, 255, 255, 255));
	close_button->setHighlightColor(makeColor(255, 255, 255, 255));
	close_button->setText("X");
	close_button->setFont(smallfont_outline);
	close_button->setHideGlyphs(true);
	close_button->setHideKeyboardGlyphs(true);
	close_button->setHideSelectors(true);
	close_button->setMenuConfirmControlType(0);
	close_button->setBackground("*#images/ui/Shop/Button_X_00.png");
	close_button->setBackgroundHighlighted("*#images/ui/Shop/Button_XHigh_00.png");
	close_button->setBackgroundActivated("*#images/ui/Shop/Button_XPress_00.png");
	close_button->setTextHighlightColor(makeColor(201, 162, 100, 255));
    close_button->setCallback([](Button& button){
        const int player = button.getOwner();
        players[player]->hud.logWindow = nullptr;
        auto parent = static_cast<Frame*>(button.getParent());
        parent->removeSelf();
        if (players[player]->gui_mode == GUI_MODE_NONE) {
            players[player]->shootmode = true;
        }
		else
		{
			players[player]->GUI.returnToPreviousActiveModule();
		}
        });

    auto help_left = frame->addField("help_left", 128);
    help_left->setSize(SDL_Rect{4, h - 32, w - 4, 32});
    help_left->setHJustify(Field::justify_t::LEFT);
    help_left->setVJustify(Field::justify_t::CENTER);
    help_left->setFont(smallfont_outline);

    frame->addImage(SDL_Rect{0,0,0,0}, 0,
		"images/system/white.png", "LogHome");
    frame->addImage(SDL_Rect{0,0,0,0}, 0,
		"images/system/white.png", "LogEnd");

    auto help_center = frame->addField("help_center", 128);
    help_center->setSize(SDL_Rect{0, h - 32, w, 32});
    help_center->setHJustify(Field::justify_t::CENTER);
    help_center->setVJustify(Field::justify_t::CENTER);
    help_center->setFont(smallfont_outline);

    frame->addImage(SDL_Rect{0,0,0,0}, 0,
		"images/system/white.png", "LogPageUp");
    frame->addImage(SDL_Rect{0,0,0,0}, 0,
		"images/system/white.png", "LogPageDown");

    auto help_right = frame->addField("help_right", 128);
    help_right->setSize(SDL_Rect{0, h - 32, w - 4, 32});
    help_right->setHJustify(Field::justify_t::RIGHT);
    help_right->setVJustify(Field::justify_t::CENTER);
    help_right->setFont(smallfont_outline);

    frame->addImage(SDL_Rect{0,0,0,0}, 0,
		"images/system/white.png", "LogScrollUp");
    frame->addImage(SDL_Rect{0,0,0,0}, 0,
		"images/system/white.png", "LogScrollDown");
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
						if ( itr->value.IsObject() )
						{
							for ( rapidjson::Value::ConstMemberIterator inner_itr = itr->value.MemberBegin();
								inner_itr != itr->value.MemberEnd(); ++inner_itr )
							{
								hoverTextStrings[inner_itr->name.GetString()] = inner_itr->value.GetString();
							}
						}
						else
						{
							hoverTextStrings[itr->name.GetString()] = itr->value.GetString();
						}
					}
				}
			}
		}
	}
}

const int NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES = 9;
const int NUM_CHARSHEET_TOOLTIP_TEXT_FIELDS = 16;
std::map<int, Field*> characterSheetTooltipTextFields[MAXPLAYERS];
std::map<int, Frame*> characterSheetTooltipTextBackingFrames[MAXPLAYERS];

static void charsheet_deselect_fn(Widget& widget) {
	if ( widget.isSelected()
		&& players[widget.getOwner()]->GUI.activeModule != Player::GUI_t::MODULE_CHARACTERSHEET
		&& !inputs.getVirtualMouse(widget.getOwner())->draw_cursor )
	{
		widget.deselect();
	}
};

void Player::CharacterSheet_t::createCharacterSheet()
{
	char name[32];
	snprintf(name, sizeof(name), "player sheet %d", player.playernum);
	if ( !gameUIFrame[player.playernum]->findFrame(name) )
	{
		characterSheetTooltipTextFields[player.playernum].clear();
		characterSheetTooltipTextBackingFrames[player.playernum].clear();

		Frame* sheetFrame = gameUIFrame[player.playernum]->addFrame(name);
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
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Window_01A_Top.png", "bg image");

			const char* titleFont = "fonts/pixel_maz.ttf#32#2";
			auto characterSheetTitleText = fullscreenBg->addField("character sheet title text", 32);
			characterSheetTitleText->setFont(titleFont);
			characterSheetTitleText->setSize(SDL_Rect{ 6, 177, 202, 32 });
			characterSheetTitleText->setText("CHARACTER SHEET");
			characterSheetTitleText->setVJustify(Field::justify_t::CENTER);
			characterSheetTitleText->setHJustify(Field::justify_t::CENTER);
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
							b->setBackgroundHighlighted("*#images/ui/CharSheet/HUD_CharSheet_ButtonHigh_00.png");
							b->setBackground("*#images/ui/CharSheet/HUD_CharSheet_Button_00.png");
						}
						else
						{
							b->setBackgroundHighlighted("*#images/ui/CharSheet/HUD_CharSheet_ButtonHighCompact_00.png");
							b->setBackground("*#images/ui/CharSheet/HUD_CharSheet_ButtonCompact_00.png");
						}
					}
				}
			});

			SDL_Rect buttonPos{0, 0, buttonFramePos.w, 40};
			auto mapButton = buttonFrame->addButton("map button");
			mapButton->setText(language[4069]);
			mapButton->setFont(buttonFont);
			mapButton->setBackground("*#images/ui/CharSheet/HUD_CharSheet_Button_00.png");
			mapButton->setSize(buttonPos);
			mapButton->setHideGlyphs(true);
			mapButton->setHideKeyboardGlyphs(true);
			mapButton->setHideSelectors(true);
			mapButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			mapButton->setColor(makeColor(255, 255, 255, 255));
			mapButton->setHighlightColor(makeColor(255, 255, 255, 255));
			mapButton->setCallback([](Button& button){
			    openMapWindow(button.getOwner());
			    if (players[button.getOwner()]->hud.mapWindow) {
	                button.deselect();
			    }
			});
			mapButton->setTickCallback(charsheet_deselect_fn);
			
			auto mapSelector = buttonFrame->addFrame("map button selector");
			mapSelector->setSize(buttonPos);
			mapSelector->setHollow(true);
			//mapSelector->setClickable(true);

			buttonPos.y = buttonPos.y + buttonPos.h + 2;
			auto logButton = buttonFrame->addButton("log button");
			logButton->setText(language[4070]);
			logButton->setFont(buttonFont);
			logButton->setBackground("*#images/ui/CharSheet/HUD_CharSheet_Button_00.png");
			logButton->setSize(buttonPos);
			logButton->setHideGlyphs(true);
			logButton->setHideKeyboardGlyphs(true);
			logButton->setHideSelectors(true);
			logButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			logButton->setColor(makeColor(255, 255, 255, 255));
			logButton->setHighlightColor(makeColor(255, 255, 255, 255));
			logButton->setCallback([](Button& button) {
			    openLogWindow(button.getOwner());
			});
			logButton->setTickCallback(charsheet_deselect_fn);
			
			auto logSelector = buttonFrame->addFrame("log button selector");
			logSelector->setSize(buttonPos);
			logSelector->setHollow(true);
			//logSelector->setClickable(true);
		}

		// game timer
		{
			const char* timerFont = "fonts/pixel_maz.ttf#32#2";
			Uint32 timerTextColor = makeColor( 188, 154, 114, 255);

			Frame* timerFrame = sheetFrame->addFrame("game timer");
			timerFrame->setSize(SDL_Rect{leftAlignX + 36, 90, 142, 26});
			timerFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos) {
				auto frame = (Frame*)(&widget);
				auto timerToggleImg = frame->findImage("timer icon img");
				auto timerSelector = frame->findButton("timer selector");
				if ( timerToggleImg && timerSelector )
				{
					if ( !players[frame->getOwner()]->characterSheet.showGameTimerAlways )
					{
						if ( timerSelector->isCurrentlyPressed() )
						{
							timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_PressOff00.png";
						}
						else if ( timerSelector->isHighlighted() || (!inputs.getVirtualMouse(widget.getOwner())->draw_cursor && timerSelector->isSelected()) )
						{
							timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_SelectOff00.png";
						}
						else
						{
							timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_UnselectOff00.png";
						}
					}
					else
					{
						if ( timerSelector->isCurrentlyPressed() )
						{
							timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_PressOn00.png";
						}
						else if ( timerSelector->isHighlighted() || (!inputs.getVirtualMouse(widget.getOwner())->draw_cursor && timerSelector->isSelected()) )
						{
							timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_SelectOn00.png";
						}
						else
						{
							timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_UnselectOn00.png";
						}
					}
				}
			});
			auto timerToggleImg = timerFrame->addImage(SDL_Rect{0, 0, 26, 26}, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_Button_Timer_SelectOff00.png", "timer icon img");
			auto timerImg = timerFrame->addImage(SDL_Rect{ 30, 0, 112, 26 }, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Timer_Backing_00.png", "timer bg img");
			auto timerText = timerFrame->addField("timer text", 32);
			timerText->setFont(timerFont);

			auto timerButton = timerFrame->addButton("timer selector");
			timerButton->setSize(SDL_Rect{ 0, 0, timerToggleImg->pos.w + timerImg->pos.w, 26 });
			timerButton->setColor(makeColor(0, 0, 0, 0));
			timerButton->setHighlightColor(makeColor(0, 0, 0, 0));
			timerButton->setHideGlyphs(true);
			timerButton->setHideKeyboardGlyphs(true);
			timerButton->setHideSelectors(true);
			timerButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			timerButton->setCallback([](Button& button){
				bool& bShowTimer = players[button.getOwner()]->characterSheet.showGameTimerAlways;
				bShowTimer = !bShowTimer;
			});
			timerButton->setTickCallback(charsheet_deselect_fn);

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
			skillsButton->setBackground("*#images/ui/CharSheet/HUD_CharSheet_ButtonWide_00.png");
			skillsButton->setSize(SDL_Rect{ 0, 0, skillsButtonFrame->getSize().w, skillsButtonFrame->getSize().h });
			skillsButton->setHideGlyphs(true);
			skillsButton->setHideKeyboardGlyphs(true);
			skillsButton->setHideSelectors(true);
			skillsButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			skillsButton->setColor(makeColor(255, 255, 255, 191));
			skillsButton->setHighlightColor(makeColor(255, 255, 255, 255));
			skillsButton->setCallback([](Button& button) {
				messagePlayer(button.getOwner(), MESSAGE_DEBUG, "%d: Skills button clicked", button.getOwner());
				players[button.getOwner()]->skillSheet.openSkillSheet();
			});
			skillsButton->setTickCallback(charsheet_deselect_fn);
		}

		// dungeon floor and level descriptor
		{
			const char* dungeonFont = "fonts/pixel_maz.ttf#32#2";
			Uint32 dungeonTextColor = makeColor( 188, 154, 114, 255);
			Frame* dungeonFloorFrame = sheetFrame->addFrame("dungeon floor frame");

			dungeonFloorFrame->setSize(SDL_Rect{ leftAlignX + 6, 118, 202, 52 });
			auto dungeonButton = dungeonFloorFrame->addButton("dungeon floor selector");
			dungeonButton->setSize(SDL_Rect{ 6, 6, 190, 44 });
			dungeonButton->setColor(makeColor(0, 0, 0, 0));
			dungeonButton->setHighlightColor(makeColor(0, 0, 0, 0));
			dungeonButton->setHideGlyphs(true);
			dungeonButton->setHideKeyboardGlyphs(true);
			dungeonButton->setHideSelectors(true);
			dungeonButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			dungeonButton->setTickCallback(charsheet_deselect_fn);

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
		Uint32 infoTextColor = makeColor( 188, 154, 114, 255);
		Uint32 classTextColor = makeColor( 74, 66, 207, 255);

		sheetFrame->addImage(SDL_Rect{ characterFrame->getSize().x, characterFrame->getSize().y - 60, 
			214, 170 }, 0xFFFFFFFF,
			"*#images/ui/CharSheet/HUD_CharSheet_Window_01A_TopCompact.png", "character info compact img");

		Frame* characterInnerFrame = characterFrame->addFrame("character info inner frame");
		characterInnerFrame->setSize(SDL_Rect{ 6, 0, 202, 104 });
		{
			//characterInnerFrame->addImage(SDL_Rect{ 0, 0, characterInnerFrame->getSize().w, characterInnerFrame->getSize().h }, 0xFFFFFFFF,
			//	"*#images/ui/CharSheet/HUD_CharSheet_Window_01A_TopTmp.png", "character info tmp img");


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
			classButton->setHideKeyboardGlyphs(true);
			classButton->setHideSelectors(true);
			classButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			classButton->setTickCallback(charsheet_deselect_fn);

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
				"*#images/ui/CharSheet/HUD_CharSheet_Sex_M_01.png", "character sex img");

			/*characterInnerFrame->addImage(SDL_Rect{ 4, 28, 194, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character race frame");*/
			auto raceButton = characterInnerFrame->addButton("character race selector");
			raceButton->setSize(SDL_Rect{ 4, 28, 194, 22 });
			raceButton->setColor(makeColor(0, 0, 0, 0));
			raceButton->setHighlightColor(makeColor(0, 0, 0, 0));
			raceButton->setHideGlyphs(true);
			raceButton->setHideKeyboardGlyphs(true);
			raceButton->setHideSelectors(true);
			raceButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			raceButton->setTickCallback(charsheet_deselect_fn);

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
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_ButtonMoney_00.png", "gold img");

			/*characterInnerFrame->addImage(SDL_Rect{ 24, 80, 156, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character gold frame");*/
			auto goldButton = characterInnerFrame->addButton("character gold selector");
			goldButton->setSize(SDL_Rect{ 22, 80, 158, 22 });
			goldButton->setColor(makeColor(0, 0, 0, 0));
			goldButton->setHighlightColor(makeColor(0, 0, 0, 0));
			goldButton->setHideGlyphs(true);
			goldButton->setHideKeyboardGlyphs(true);
			goldButton->setHideSelectors(true);
			goldButton->setMenuConfirmControlType(0);
			goldButton->setTickCallback(charsheet_deselect_fn);
		}

		{
			Frame* statsFrame = sheetFrame->addFrame("stats");
			const int statsFrameHeight = 182;
			statsFrame->setSize(SDL_Rect{ leftAlignX, 344, bgWidth, statsFrameHeight });
			statsFrame->addImage(SDL_Rect{ 0, 0, statsFrame->getSize().w, statsFrame->getSize().h },
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Window_01B_BotA.png", "stats bg img");

			Frame* statsInnerFrame = statsFrame->addFrame("stats inner frame");
			statsInnerFrame->setSize(SDL_Rect{ 0, 0, statsFrame->getSize().w, statsFrame->getSize().h });

			SDL_Rect iconPos{ 20, 8, 24, 24 };
			const int headingLeftX = iconPos.x + iconPos.w + 4;
			const int baseStatLeftX = headingLeftX + 32;
			const int modifiedStatLeftX = baseStatLeftX + 64;
			SDL_Rect textPos{ headingLeftX, iconPos.y, 40, iconPos.h };
			Uint32 statTextColor = hudColors.characterSheetNeutral;

			const char* statFont = "fonts/pixel_maz.ttf#32#2";
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_STR_00.png", "str icon");
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
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("str text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("str button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}
			const int rowSpacing = 4;
			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_DEX_00.png", "dex icon");
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
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("dex text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("2");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("dex button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_CON_00.png", "con icon");
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
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("con button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_INT_00.png", "int icon");
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
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("int text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("int button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_PER_00.png", "per icon");
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
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("per text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("per button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_CHA_00.png", "chr icon");
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
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("chr text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("chr button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}
		}

		{
			Frame* attributesFrame = sheetFrame->addFrame("attributes");
			attributesFrame->setSize(SDL_Rect{ leftAlignX, 550, bgWidth, 182 });

			attributesFrame->addImage(SDL_Rect{ 0, 0, attributesFrame->getSize().w, attributesFrame->getSize().h },
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Window_01B_BotB.png", "attributes bg img");

			Frame* attributesInnerFrame = attributesFrame->addFrame("attributes inner frame");
			attributesInnerFrame->setSize(SDL_Rect{ 0, 0, attributesFrame->getSize().w, attributesFrame->getSize().h });

			SDL_Rect iconPos{ 20, 8, 24, 24 };
			const int headingLeftX = iconPos.x + iconPos.w + 4;
			const int baseStatLeftX = headingLeftX + 48;
			SDL_Rect textPos{ headingLeftX, iconPos.y, 80, iconPos.h };
			Uint32 statTextColor = hudColors.characterSheetNeutral;

			const char* attributeFont = "fonts/pixel_maz.ttf#32#2";
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_ATT_00.png", "atk icon");
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

				auto attributeButton = attributesInnerFrame->addButton("atk button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			const int rowSpacing = 4;
			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_AC_00.png", "ac icon");
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

				auto attributeButton = attributesInnerFrame->addButton("ac button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_SPWR_00.png", "pwr icon");
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

				auto attributeButton = attributesInnerFrame->addButton("pow button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_RES_00.png", "res icon");
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

				auto attributeButton = attributesInnerFrame->addButton("res button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_REGEN_00.png", "regen icon");
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
				textPos.x = baseStatLeftX + 0;
				textDiv->setSize(textPos);
				textDiv->setText(" ");
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

				auto attributeButton = attributesInnerFrame->addButton("rgn button");
				const int fullWidth = attributesFrame->getSize().w - 34 + 12;
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34 - 50, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);

				auto attributeButton2 = attributesInnerFrame->addButton("rgn mp button");
				attributeButton2->setSize(SDL_Rect{ attributeButton->getSize().x + attributeButton->getSize().w,
					iconPos.y + 2, fullWidth - (attributeButton->getSize().x + attributeButton->getSize().w), iconPos.h - 2 });
				attributeButton2->setColor(makeColor(0, 0, 0, 0));
				attributeButton2->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton2->setHideGlyphs(true);
				attributeButton2->setHideKeyboardGlyphs(true);
				attributeButton2->setHideSelectors(true);
				attributeButton2->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton2->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_WGT_00.png", "weight icon");
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

				auto attributeButton = attributesInnerFrame->addButton("wgt button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}
		}

		{
		auto tooltipFrame = sheetFrame->addFrame("sheet tooltip");
		tooltipFrame->setSize(SDL_Rect{ leftAlignX - 200, 0, 200, 200 });
		tooltipFrame->setHollow(true);
		tooltipFrame->setInheritParentFrameOpacity(false);
		tooltipFrame->setDisabled(true);
		Uint32 color = makeColor(255, 255, 255, 255);
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_00.png", skillsheetEffectBackgroundImages[TOP].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_L_00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_R_00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			makeColor(22, 24, 29, 255), "images/system/white.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_BL_00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_BR_00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_B_00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
		imageSetWidthHeight9x9(tooltipFrame, skillsheetEffectBackgroundImages);
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, 200, 200 }, skillsheetEffectBackgroundImages);
		auto txt = tooltipFrame->addField("tooltip text", 1024);
		auto txtRightAlignHint = tooltipFrame->addField("tooltip text right align hint", 128);
		const char* tooltipFont = "fonts/pixel_maz_multiline.ttf#16#2";
		txt->setFont(tooltipFont);
		txt->setColor(makeColor(188, 154, 114, 255));
		txtRightAlignHint->setFont(tooltipFont);
		txtRightAlignHint->setColor(hudColors.characterSheetFaintText);
		auto glyph1 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 1");
		glyph1->disabled = true;
		auto glyph2 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 2");
		glyph2->disabled = true;
		auto glyph3 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 3");
		glyph3->disabled = true;
		auto glyph4 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 4");
		glyph4->disabled = true;

		for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_TEXT_FIELDS; ++i )
		{
			char txtEntryFieldName[64] = "";
			snprintf(txtEntryFieldName, sizeof(txtEntryFieldName), "txt %d", i);
			auto txtEntry = tooltipFrame->addField(txtEntryFieldName, 1024);
			txtEntry->setFont(tooltipFont);
			txtEntry->setDisabled(true);
			txtEntry->setVJustify(Field::justify_t::CENTER);
			txtEntry->setColor(makeColor(188, 154, 114, 255));
			txtEntry->setOntop(true);

			characterSheetTooltipTextFields[player.playernum][i] = txtEntry;
		}

		auto div = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
			makeColor( 49, 53, 61, 255),
			"images/system/white.png", "tooltip divider 1");
		div->disabled = true;
		div = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
			makeColor( 49, 53, 61, 255),
			"images/system/white.png", "tooltip divider 2");
		div->disabled = true;


		for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++i )
		{
			char backingFrameName[64] = "";
			snprintf(backingFrameName, sizeof(backingFrameName), "txt value backing frame %d", i);
			auto txtValueBackingFrame = tooltipFrame->addFrame(backingFrameName);
			txtValueBackingFrame->setSize(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, tooltipFrame->getSize().h });
			txtValueBackingFrame->setDisabled(true);
			Uint32 color = makeColor(51, 33, 26, 255);
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_TL00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_TR00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_T00.png", skillsheetEffectBackgroundImages[TOP].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_L00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_R00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_M00.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_BL00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_BR00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
			txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_B00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
			imageSetWidthHeight9x9(txtValueBackingFrame, skillsheetEffectBackgroundImages);

			characterSheetTooltipTextBackingFrames[player.playernum][i] = txtValueBackingFrame;
		}
		}
	}
}

void Player::GUIDropdown_t::activateSelection(const std::string& name, const int option)
{
	auto& dropdown = allDropDowns[name];
	if ( dropdown.options[option].action == "no_action" )
	{
		messagePlayer(player.playernum, MESSAGE_DEBUG, "[Dropdowns]: Warning, no action for %s : option %d", name.c_str(), option);
	}
	else
	{
		if ( dropdown.options[option].action == "gold_drop_10" )
		{
			char dropCommand[32] = "";
			snprintf(dropCommand, sizeof(dropCommand), "/dropgold %d %d", player.playernum, 10);
			consoleCommand(dropCommand);
		}
		else if ( dropdown.options[option].action == "gold_drop_100" )
		{
			char dropCommand[32] = "";
			snprintf(dropCommand, sizeof(dropCommand), "/dropgold %d %d", player.playernum, 100);
			consoleCommand(dropCommand);
		}
		else if ( dropdown.options[option].action == "gold_drop_1000" )
		{
			char dropCommand[32] = "";
			snprintf(dropCommand, sizeof(dropCommand), "/dropgold %d %d", player.playernum, 1000);
			consoleCommand(dropCommand);
		}
		else if ( dropdown.options[option].action == "gold_drop_all" )
		{
			char dropCommand[32] = "";
			snprintf(dropCommand, sizeof(dropCommand), "/dropgold %d %d", player.playernum, std::max(0, stats[player.playernum]->GOLD));
			consoleCommand(dropCommand);
		}
		messagePlayer(player.playernum, MESSAGE_DEBUG, "[Dropdowns]: Executing action '%s' for [%s] : option %d",
			dropdown.options[option].action.c_str(), name.c_str(), option);
	}
}

bool Player::GUIDropdown_t::getDropDownAlignRight(const std::string& name)
{
	bool invert = false;
	if ( name == "drop_gold" )
	{
		if ( player.characterSheet.panelJustify == Player::PANEL_JUSTIFY_RIGHT 
			&& player.bUseCompactGUIHeight() )
		{
			invert = true;
		}
	}
	else if ( name == "chest_interact" )
	{
		if ( player.inventoryUI.bCompactView )
		{
			if ( player.inventoryUI.inventoryPanelJustify != PanelJustify_t::PANEL_JUSTIFY_RIGHT )
			{
				invert = true;
			}
		}
	}
	else if ( name == "item_interact" || name == "spell_interact" )
	{
		if ( player.inventoryUI.bCompactView )
		{
			Item* item = uidToItem(dropDownItem);
			if ( item && player.paperDoll.isItemOnDoll(*item) )
			{
				if ( player.inventoryUI.paperDollPanelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
				{
					invert = true;
				}
			}
			else if ( item && itemCategory(item) == SPELL_CAT )
			{
				if ( player.inventoryUI.spellPanel.panelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
				{
					invert = true;
				}
			}
			else
			{
				// normal inventory items
				if ( player.inventoryUI.inventoryPanelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
				{
					invert = true;
				}
			}
		}
	}
	return (invert ? !allDropDowns[name].alignRight : allDropDowns[name].alignRight);
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
		queuedElement = SHEET_UNSELECTED;
		return;
	}

	bool bCompactView = false;
	if ( (keystatus[SDL_SCANCODE_U] && enableDebugKeys) || player.bUseCompactGUIHeight() )
	{
		bCompactView = true;
	}

	bool hideAndExit = false;
	if ( nohud || player.shootmode || !(player.gui_mode == GUI_MODE_INVENTORY || player.gui_mode == GUI_MODE_SHOP) )
	{
		hideAndExit = true;
	}
	else if ( bCompactView && player.hud.compactLayoutMode != Player::HUD_t::COMPACT_LAYOUT_CHARSHEET )
	{
		hideAndExit = true;
	}

	sheetFrame->setDisabled(false);

	if ( bCompactView )
	{
		// justify opposite of inventory
		panelJustify = (player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT) 
			? Player::PANEL_JUSTIFY_RIGHT : Player::PANEL_JUSTIFY_LEFT;
	}
	else
	{
		panelJustify = Player::PANEL_JUSTIFY_RIGHT;
	}

	if ( player.inventoryUI.slideOutPercent <= .0001 )
	{
		isInteractable = !hideAndExit;
		if ( isInteractable && queuedElement != SHEET_UNSELECTED )
		{
			const bool updateCursor = true;
			const bool usingMouse = false;
			selectElement(queuedElement, usingMouse, updateCursor);
			queuedElement = SHEET_UNSELECTED;
		}
	}
	else
	{
		isInteractable = false;
	}

	// resize elements for splitscreen
	{
		auto characterInfoFrame = sheetFrame->findFrame("character info");
		const int bgWidth = 208;
		const int leftAlignX = sheetFrame->getSize().w - bgWidth + player.inventoryUI.slideOutPercent * player.inventoryUI.slideOutWidth;
		int compactAlignX1 = -player.inventoryUI.slideOutPercent * player.inventoryUI.slideOutWidth;
		int compactAlignX2 = sheetFrame->getSize().w + player.inventoryUI.slideOutPercent * player.inventoryUI.slideOutWidth;
		auto compactImgBg = sheetFrame->findImage("character info compact img");
		Frame* dungeonFloorFrame = sheetFrame->findFrame("dungeon floor frame");
		Frame* buttonFrame = sheetFrame->findFrame("log map buttons");
		Frame* fullscreenBg = sheetFrame->findFrame("sheet bg fullscreen");
		Frame* timerFrame = sheetFrame->findFrame("game timer");
		Frame* skillsButtonFrame = sheetFrame->findFrame("skills button frame");

		auto statsFrame = sheetFrame->findFrame("stats");
		auto attributesFrame = sheetFrame->findFrame("attributes");

		if ( bCompactView )
		{
			// compact view
			sheetDisplayType = CHARSHEET_DISPLAY_COMPACT;

			compactImgBg->disabled = false;
			compactImgBg->pos.x = (panelJustify == PANEL_JUSTIFY_RIGHT ? compactAlignX1 : compactAlignX2 - compactImgBg->pos.w);
			compactImgBg->pos.y = 0;

			SDL_Rect pos = characterInfoFrame->getSize();
			pos.x = (panelJustify == PANEL_JUSTIFY_RIGHT ? compactAlignX1 : compactAlignX2 - pos.w);
			pos.y = compactImgBg->pos.y + 60;
			characterInfoFrame->setSize(pos);

			SDL_Rect dungeonFloorFramePos = dungeonFloorFrame->getSize();
			dungeonFloorFramePos.x = pos.x + 6;
			dungeonFloorFramePos.y = pos.y - 56;
			dungeonFloorFrame->setSize(dungeonFloorFramePos);

			SDL_Rect buttonFramePos = buttonFrame->getSize();
			buttonFramePos.w = 198;
			buttonFramePos.h = 40;
			buttonFramePos.x = (panelJustify == PANEL_JUSTIFY_RIGHT ? compactAlignX1 + 8 : compactAlignX2 - buttonFramePos.w - 8);
			buttonFramePos.y = compactImgBg->pos.y + compactImgBg->pos.h + 1;
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
			statsPos.x = (panelJustify == PANEL_JUSTIFY_RIGHT ? compactAlignX2 - statsPos.w : compactAlignX1);
			statsPos.y = 0;
			statsFrame->setSize(statsPos);

			auto attributesPos = attributesFrame->getSize();
			attributesPos.x = (panelJustify == PANEL_JUSTIFY_RIGHT ? compactAlignX2 - attributesPos.w : compactAlignX1);
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
			fullscreenBgPos.x = sheetFrame->getSize().w - fullscreenBgPos.w + player.inventoryUI.slideOutPercent * player.inventoryUI.slideOutWidth;
			fullscreenBg->setSize(fullscreenBgPos);
			compactImgBg->disabled = true;

			auto attributesPos = attributesFrame->getSize();
			attributesPos.x = sheetFrame->getSize().w - attributesPos.w + player.inventoryUI.slideOutPercent * player.inventoryUI.slideOutWidth;
			attributesPos.y = sheetFrame->getSize().h - attributesPos.h;
			attributesFrame->setSize(attributesPos);

			auto statsPos = statsFrame->getSize();
			statsPos.x = sheetFrame->getSize().w - statsPos.w + player.inventoryUI.slideOutPercent * player.inventoryUI.slideOutWidth;
			statsPos.y = attributesPos.y - statsPos.h + 4; // 4 pixels below top edge of attributes pane
			statsFrame->setSize(statsPos);
		}
	}

	updateCharacterSheetTooltip(SHEET_UNSELECTED, SDL_Rect{ 0, 0, 0, 0 }); // to reset the tooltip from displaying.
	if ( hideAndExit )
	{
		queuedElement = SHEET_UNSELECTED;
		sheetFrame->setDisabled(true);
		return;
	}

	player.GUI.handleCharacterSheetMovement();

	{
		auto characterInfoFrame = sheetFrame->findFrame("character info");
		assert(characterInfoFrame);
		auto characterInnerFrame = characterInfoFrame->findFrame("character info inner frame");
		assert(characterInnerFrame);
		auto statsFrame = sheetFrame->findFrame("stats");
		assert(statsFrame);
		auto statsInnerFrame = statsFrame->findFrame("stats inner frame");
		assert(statsInnerFrame);
		auto attributesFrame = sheetFrame->findFrame("attributes");
		assert(attributesFrame);
		auto attributesInnerFrame = attributesFrame->findFrame("attributes inner frame");
		assert(attributesInnerFrame);
		SheetElements targetElement = SHEET_ENUM_END;
		auto logButton = sheetFrame->findFrame("log map buttons")->findButton("log button");
		auto mapButton = sheetFrame->findFrame("log map buttons")->findButton("map button");
		auto skillsButton = sheetFrame->findFrame("skills button frame")->findButton("skills button");
		Button* strButton = statsInnerFrame->findButton("str button");
		Button* dexButton = statsInnerFrame->findButton("dex button");
		Button* conButton = statsInnerFrame->findButton("con button");
		Button* intButton = statsInnerFrame->findButton("int button");
		Button* perButton = statsInnerFrame->findButton("per button");
		Button* chrButton = statsInnerFrame->findButton("chr button");
		Button* atkButton = attributesInnerFrame->findButton("atk button");
		Button* acButton = attributesInnerFrame->findButton("ac button");
		Button* powButton = attributesInnerFrame->findButton("pow button");
		Button* resButton = attributesInnerFrame->findButton("res button");
		Button* rgnButton = attributesInnerFrame->findButton("rgn button");
		Button* rgnMpButton = attributesInnerFrame->findButton("rgn mp button");
		Button* wgtButton = attributesInnerFrame->findButton("wgt button");

		auto sheetTimer = sheetFrame->findFrame("game timer");
		if ( bCompactView )
		{
			sheetTimer->setDisabled(true);
		}
		else
		{
			sheetTimer->setDisabled(false);
		}

		if ( inputs.getVirtualMouse(player.playernum)->draw_cursor
			&& (!player.GUI.isDropdownActive())
			&& isInteractable )
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
			else if ( !sheetTimer->isDisabled() && sheetTimer->findButton("timer selector")->isHighlighted() )
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
			else if ( strButton->isHighlighted() )
			{
				targetElement = SHEET_STR;
			}
			else if ( dexButton->isHighlighted() )
			{
				targetElement = SHEET_DEX;
			}
			else if ( conButton->isHighlighted() )
			{
				targetElement = SHEET_CON;
			}
			else if ( intButton->isHighlighted() )
			{
				targetElement = SHEET_INT;
			}
			else if ( perButton->isHighlighted() )
			{
				targetElement = SHEET_PER;
			}
			else if ( chrButton->isHighlighted() )
			{
				targetElement = SHEET_CHR;
			}
			else if ( atkButton->isHighlighted() )
			{
				targetElement = SHEET_ATK;
			}
			else if ( acButton->isHighlighted() )
			{
				targetElement = SHEET_AC;
			}
			else if ( powButton->isHighlighted() )
			{
				targetElement = SHEET_POW;
			}
			else if ( resButton->isHighlighted() )
			{
				targetElement = SHEET_RES;
			}
			else if ( rgnButton->isHighlighted() )
			{
				targetElement = SHEET_RGN;
			}
			else if ( rgnMpButton->isHighlighted() )
			{
				targetElement = SHEET_RGN_MP;
			}
			else if ( wgtButton->isHighlighted() )
			{
				targetElement = SHEET_WGT;
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
				if ( player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
					&& !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick )
				{
					// no moused over objects, deactivate the cursor.
					player.GUI.activateModule(Player::GUI_t::MODULE_NONE);
				}
			}
		}
		//messagePlayer(0, "%d", player.GUI.activeModule);
		// players[player.playernum]->GUI.warpControllerToModule(false); - use this for controller input
	}

	updateGameTimer();
	updateStats();
	updateAttributes();
	updateCharacterInfo();
}

std::map<std::string, Player::GUIDropdown_t::DropDown_t> Player::GUIDropdown_t::allDropDowns;

bool Player::GUIDropdown_t::set(const std::string name)
{
	if ( allDropDowns.find(name) == allDropDowns.end() )
	{
		printlog("[Dropdown]: Could not find dropdown name %s", name.c_str());
		return false;
	}

	if ( currentName != name )
	{
		create(name);
	}
	currentName = name;
	return true;
}

void Player::GUIDropdown_t::process()
{
	bClosedThisTick = false;
	if ( dropdownBlockClickFrame )
	{
		dropdownBlockClickFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
			players[player.playernum]->camera_virtualy1(),
			players[player.playernum]->camera_virtualWidth(),
			players[player.playernum]->camera_virtualHeight() });
	}
	if ( !dropdownFrame ) { return; }
	
	if ( !bOpen )
	{
		dropdownFrame->setDisabled(true);
		dropDownOptionSelected = -1;
		dropDownItem = 0;
		dropDownX = 0;
		dropDownY = 0;
		dropDownToggleClick = false;
		return;
	}
	else
	{
		dropdownFrame->setDisabled(false);
	}

	auto highlightImageMid = dropdownFrame->findImage("interact selected highlight mid");
	highlightImageMid->disabled = true;
	highlightImageMid->color = hudColors.itemContextMenuOptionSelectedImg;
	auto highlightImageLeft = dropdownFrame->findImage("interact selected highlight left");
	highlightImageLeft->disabled = true;
	highlightImageLeft->color = hudColors.itemContextMenuOptionSelectedImg;
	auto highlightImageRight = dropdownFrame->findImage("interact selected highlight right");
	highlightImageRight->disabled = true;
	highlightImageRight->color = hudColors.itemContextMenuOptionSelectedImg;

	size_t index = 0;
	unsigned int maxWidth = 0;
	if ( auto interactText = dropdownFrame->findField("interact text") )
	{
		interactText->setColor(hudColors.itemContextMenuHeadingText);
		if ( auto textGet = Text::get(interactText->getText(), interactText->getFont(),
			interactText->getTextColor(), interactText->getOutlineColor()) )
		{
			maxWidth = textGet->getWidth();
		}
	}
	int maxHeight = 0;
	const int textPaddingX = 8;

	auto dropDown = allDropDowns[currentName];
	std::vector<ItemContextMenuPrompts> contextOptions;
	Item* item = uidToItem(dropDownItem);
	if ( currentName == "chest_interact" )
	{
		list_t* chest_inventory = nullptr;
		if ( openedChest[player.playernum] )
		{
			if ( multiplayer == CLIENT )
			{
				chest_inventory = &chestInv[player.playernum];
			}
			else if ( openedChest[player.playernum]->children.first && openedChest[player.playernum]->children.first->element )
			{
				chest_inventory = (list_t*)openedChest[player.playernum]->children.first->element;
			}
		}

		if ( chest_inventory )
		{
			for ( node_t* node = chest_inventory->first; node != NULL; node = node->next )
			{
				Item* chestItem = (Item*)node->element;
				if ( !chestItem )
				{
					continue;
				}
				if ( chestItem->uid == dropDownItem )
				{
					item = chestItem;
					break;
				}
			}
		}
	}

	if ( dropDownItem != 0 && !item )
	{
		close();
		return;
	}
	if ( currentName == "item_interact" || currentName == "hotbar_interact" 
		|| currentName == "chest_interact"
		|| currentName == "spell_interact" )
	{
		if ( item )
		{
			dropDown.options.clear();
			contextOptions = getContextMenuOptionsForItem(player.playernum, item);
			if ( currentName == "hotbar_interact" )
			{
				contextOptions.push_back(PROMPT_CLEAR_HOTBAR_SLOT);
			}
			if ( player.inventoryUI.useItemDropdownOnGamepad == Inventory_t::GAMEPAD_DROPDOWN_FULL )
			{
				for ( auto it = contextOptions.begin(); it != contextOptions.end(); )
				{
					if ( (*it) == PROMPT_APPRAISE )
					{
						it = contextOptions.erase(it);
						continue;
					}
					++it;
				}
			}
			else if ( player.inventoryUI.useItemDropdownOnGamepad == Inventory_t::GAMEPAD_DROPDOWN_COMPACT )
			{
				for ( auto it = contextOptions.begin(); it != contextOptions.end(); )
				{
					if ( (*it) == PROMPT_APPRAISE || (*it) == PROMPT_DROP )
					{
						it = contextOptions.erase(it);
						continue;
					}
					++it;
				}
			}
			for ( auto option : contextOptions )
			{
				dropDown.options.push_back(DropdownOption_t(getContextMenuLangEntry(player.playernum, option, *item), "", "", ""));
			}
		}
	}
	if ( dropDown.module != Player::GUI_t::MODULE_NONE )
	{
		if ( dropDown.module != player.GUI.activeModule )
		{
			close();
			return;
		}
	}

	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		if ( Input::inputs[player.playernum].consumeBinaryToggle("InventoryMoveDown")
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
		{
			++dropDownOptionSelected;
			if ( dropDownOptionSelected >= dropDown.options.size() )
			{
				dropDownOptionSelected = 0;
			}
		}
		else if ( Input::inputs[player.playernum].consumeBinaryToggle("InventoryMoveUp")
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
		{
			--dropDownOptionSelected;
			if ( dropDownOptionSelected < 0 )
			{
				dropDownOptionSelected = dropDown.options.size() - 1;
			}
		}
	}

	std::vector<std::tuple<Frame::image_t*, Field*, Frame::image_t*>> optionFrames;
	index = 1;
	for ( auto& option : dropDown.options )
	{
		char glyphname[32] = "";
		snprintf(glyphname, sizeof(glyphname), "glyph %d", index);
		char optionname[32] = "";
		snprintf(optionname, sizeof(optionname), "interact option %d", index);

		auto img = dropdownFrame->findImage(glyphname);
		auto txt = dropdownFrame->findField(optionname);
		optionFrames.push_back(std::make_tuple(img, txt, nullptr));
		txt->setColor(hudColors.itemContextMenuOptionText);
		//txt->setColor(makeColor(148, 82, 3, 255));
		txt->setDisabled(false);
		img->disabled = true;

		if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			if ( dropDownOptionSelected + 1 == index )
			{
				img->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuConfirm");
				if ( auto imgGet = Image::get(img->path.c_str()) )
				{
					img->pos.w = imgGet->getWidth();
					img->pos.h = imgGet->getHeight();
					img->disabled = false;
				}
			}
		}
		/*if ( option.controllerGlyph != "" && inputs.getVirtualMouse(player.playernum)->lastMovementFromController )
		{
			img->path = Input::inputs[player.playernum].getGlyphPathForBinding(option.controllerGlyph.c_str());
			img->disabled = false;
		}
		else if ( option.keyboardGlyph != "" && inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			img->path = Input::inputs[player.playernum].getGlyphPathForBinding(option.keyboardGlyph.c_str());
			img->disabled = false;
		}*/

		txt->setText(option.text.c_str());
		if ( auto textGet = Text::get(txt->getText(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor()) )
		{
			maxWidth = std::max(textGet->getWidth() + 40, maxWidth);

			SDL_Rect size = txt->getSize();
			size.w = textGet->getWidth();
			if ( img->disabled || true )
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

		img->pos.x = txt->getSize().x - 8;
		img->pos.y = txt->getSize().y + txt->getSize().h / 2 - img->pos.h / 2;

		maxHeight = std::max(std::max(txt->getSize().y + txt->getSize().h, img->pos.y + img->pos.h), maxHeight);
		++index;
	}

	const int rightClickProtectBuffer = (right_click_protect ? 0 : 10);
	const Sint32 mousex = (inputs.getMouse(player.playernum, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	const Sint32 mousey = (inputs.getMouse(player.playernum, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;

	index = 0;
	bool alignRight = getDropDownAlignRight(currentName);
	for ( auto& optionGlyphTextBacking : optionFrames )
	{
		auto img = std::get<0>(optionGlyphTextBacking);
		auto txt = std::get<1>(optionGlyphTextBacking);

		if ( txt->getHJustify() == Field::justify_t::CENTER || true )
		{
			SDL_Rect size = txt->getSize();
			size.w = maxWidth;
			txt->setSize(size);
		}

		if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			auto ml = dropdownFrame->findImage("interact middle left");
			SDL_Rect absoluteSize = txt->getAbsoluteSize();
			absoluteSize.x -= (4 + ml->pos.w) + (alignRight ? rightClickProtectBuffer : 0);
			absoluteSize.w += ((4 + ml->pos.w) * 2 + rightClickProtectBuffer);
			absoluteSize.y += 4;
			absoluteSize.h -= 4;
			if ( mousex >= absoluteSize.x && mousex < absoluteSize.x + absoluteSize.w
				&& mousey >= absoluteSize.y && mousey < absoluteSize.y + absoluteSize.h )
			{
				dropDownOptionSelected = index;
			}
		}
		++index;
	}

	int textStartX = textPaddingX;
	//if ( optionFrames.size() > 0 )
	//{
	//	auto img = std::get<0>(optionFrames[0]);
	//	textStartX += (img->disabled ? 0 : img->pos.x + img->pos.w);
	//}
	const int frameWidth = maxWidth + textStartX + textPaddingX;

	SDL_Rect frameSize = dropdownFrame->getSize();
	frameSize.x = dropDownX;
	frameSize.y = dropDownY;

	// position the frame elements
	int interimHeight = 0;
	{
		auto tl = dropdownFrame->findImage("interact top left");
		auto tmid = dropdownFrame->findImage("interact top background");
		tmid->pos.w = frameWidth - tl->pos.w * 2;
		auto tr = dropdownFrame->findImage("interact top right");
		tr->pos.x = tmid->pos.x + tmid->pos.w;

		interimHeight = tmid->pos.y + tmid->pos.h;
		frameSize.w = tr->pos.x + tr->pos.w;
	}
	{
		const int middleOffsetY = 13;
		auto ml = dropdownFrame->findImage("interact middle left");
		ml->pos.h = maxHeight - interimHeight - middleOffsetY;
		auto mmid = dropdownFrame->findImage("interact middle background");
		mmid->pos.h = ml->pos.h;
		mmid->pos.w = frameWidth - (ml->pos.w) * 2;
		mmid->color = hudColors.itemContextMenuOptionImg;
		auto mr = dropdownFrame->findImage("interact middle right");
		mr->pos.h = ml->pos.h;
		mr->pos.x = mmid->pos.x + mmid->pos.w;

		interimHeight = mmid->pos.y + mmid->pos.h;
	}
	{
		auto bl = dropdownFrame->findImage("interact bottom left");
		bl->pos.y = interimHeight;
		auto bmid = dropdownFrame->findImage("interact bottom background");
		bmid->pos.y = interimHeight;
		bmid->pos.w = frameWidth - bl->pos.w * 2;
		auto br = dropdownFrame->findImage("interact bottom right");
		br->pos.y = interimHeight;
		br->pos.x = bmid->pos.x + bmid->pos.w;

		frameSize.h = br->pos.y + br->pos.h;
	}

	if ( auto interactText = dropdownFrame->findField("interact text") )
	{
		SDL_Rect size = interactText->getSize();
		size.x = 0;
		size.w = frameSize.w;
		interactText->setSize(size);
	}

	if ( !alignRight )
	{
		frameSize.x -= frameSize.w;
	}
	if ( frameSize.y + frameSize.h > player.camera_virtualHeight() )
	{
		frameSize.y -= (frameSize.y + frameSize.h) - player.camera_virtualy2();
	}
	dropdownFrame->setSize(frameSize);

	if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		SDL_Rect absoluteSize = dropdownFrame->getAbsoluteSize();
		// right click protect uses exact border, else there is 10 px buffer
		absoluteSize.x -= (alignRight ? rightClickProtectBuffer : 0);
		absoluteSize.w += rightClickProtectBuffer;
	if ( !(mousex >= absoluteSize.x && mousex < absoluteSize.x + absoluteSize.w
		&& mousey >= absoluteSize.y && mousey < absoluteSize.y + absoluteSize.h) )
	{
		dropDownOptionSelected = -1;
	}
	}

	if ( dropDownOptionSelected >= 0 && dropDownOptionSelected < dropDown.options.size() )
	{
		auto txt = std::get<1>(optionFrames[dropDownOptionSelected]);
		txt->setColor(hudColors.itemContextMenuOptionSelectedText);
		SDL_Rect size = txt->getSize();
		size.x -= 4;
		size.w += 2 * 4;
		size.y += 3;
		highlightImageMid->pos = size;
		highlightImageMid->pos.x += highlightImageLeft->pos.w;
		highlightImageMid->pos.w -= 2 * highlightImageLeft->pos.w;
		highlightImageMid->pos.h = highlightImageLeft->pos.h;
		highlightImageMid->disabled = false;

		highlightImageLeft->pos.x = highlightImageMid->pos.x - highlightImageLeft->pos.w;
		highlightImageLeft->pos.y = highlightImageMid->pos.y;
		highlightImageLeft->disabled = false;
		highlightImageRight->pos.x = highlightImageMid->pos.x + highlightImageMid->pos.w;
		highlightImageRight->disabled = false;
		highlightImageRight->pos.y = highlightImageMid->pos.y;
	}

	bool activate = false;
	if ( !dropDownToggleClick && !Input::inputs[player.playernum].binary("MenuRightClick") )
	{
		activate = true;
	}
	else if ( dropDownToggleClick )
	{
		if ( Input::inputs[player.playernum].consumeBinaryToggle("MenuRightClick") )
		{
			close();
		}
		else if ( Input::inputs[player.playernum].consumeBinaryToggle("MenuConfirm") )
		{
			activate = true;
		}
		else if ( Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel") )
		{
			close();
		}
	}

	if ( !(player.bControlEnabled && !gamePaused && !player.usingCommand()) )
	{
		close();
	}
	else if ( activate )
	{
		if ( dropDownOptionSelected >= 0 && dropDownOptionSelected < dropDown.options.size() )
		{
			if ( currentName == "item_interact" || currentName == "hotbar_interact" 
				|| currentName == "chest_interact" || currentName == "spell_interact" )
			{
				if ( item && dropDownOptionSelected < contextOptions.size() )
				{
					if ( contextOptions[dropDownOptionSelected] == ItemContextMenuPrompts::PROMPT_DROP 
						&& player.paperDoll.isItemOnDoll(*item) )
					{
						// need to unequip
						player.inventoryUI.activateItemContextMenuOption(item, ItemContextMenuPrompts::PROMPT_UNEQUIP_FOR_DROP);
						player.paperDoll.updateSlots();
						if ( player.paperDoll.isItemOnDoll(*item) )
						{
							// couldn't unequip, no more actions
						}
						else
						{
							// successfully unequipped, let's drop it.
							bool droppedAll = false;
							while ( item && item->count > 1 )
							{
								droppedAll = dropItem(item, player.playernum);
								if ( droppedAll )
								{
									item = nullptr;
								}
							}
							if ( !droppedAll )
							{
								dropItem(item, player.playernum);
							}
						}
					}
					else
					{
						player.inventoryUI.activateItemContextMenuOption(item, contextOptions[dropDownOptionSelected]);
					}
				}
			}
			else
			{
				activateSelection(currentName, dropDownOptionSelected);
			}
		}
		//Close the menu.
		close();
	}
}

void Player::GUIDropdown_t::close()
{
	if ( dropdownFrame )
	{
		dropdownFrame->setHollow(true);
		dropdownFrame->removeSelf();
		dropdownFrame = nullptr;
	}
	if ( dropdownBlockClickFrame )
	{
		dropdownBlockClickFrame->setHollow(true);
		dropdownBlockClickFrame->removeSelf();
		dropdownBlockClickFrame = nullptr;
	}
	dropDownOptionSelected = 0;
	dropDownItem = 0;
	bOpen = false;
	dropDownToggleClick = false;
	currentName = "";
	bClosedThisTick = true;
}


void Player::GUI_t::closeDropdowns()
{
	if ( dropdownMenu.bOpen )
	{
		dropdownMenu.close();
	}
	if ( inputs.getUIInteraction(player.playernum)->itemMenuOpen )
	{
		inputs.getUIInteraction(player.playernum)->itemMenuOpen = false;
		player.inventoryUI.updateItemContextMenu();
	}
}
bool Player::GUI_t::isDropdownActive()
{
	if ( dropdownMenu.bOpen )
	{
		return true;
	}
	if ( inputs.getUIInteraction(player.playernum)->itemMenuOpen )
	{
		return true;
	}
	return false;
}

void Player::GUIDropdown_t::open(const std::string name)
{
	set(name);
	assert(dropdownFrame);
	if ( bOpen )
	{
		return;
	}

	bOpen = true;
	bClosedThisTick = false;

	// clear inventory item context menu stuff
	bool& itemMenuOpen = inputs.getUIInteraction(player.playernum)->itemMenuOpen;
	itemMenuOpen = false;

	if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		dropDownX = (inputs.getMouse(player.playernum, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX + (getDropDownAlignRight(name) ? 8 : -8);
		dropDownY = (inputs.getMouse(player.playernum, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
		if ( auto interactMenuTop = dropdownFrame->findImage("interact top background") )
		{
			// 10px is slot half height, minus the top interact text height
			// mouse will be situated halfway in first menu option
			dropDownY -= (interactMenuTop->pos.h + 10 + 2);
		}
		if ( dropDownX % 2 == 1 )
		{
			++dropDownX; // even pixel adjustment
		}
		if ( dropDownY % 2 == 1 )
		{
			++dropDownY; // even pixel adjustment
		}
	}
	dropDownOptionSelected = 0;
	if ( allDropDowns[name].defaultOption > 0 && allDropDowns[name].options.size() > allDropDowns[name].defaultOption )
	{
		if ( auto txt = dropdownFrame->findField("interact option 1") )
		{
			dropDownY -= txt->getSize().h;
			dropDownOptionSelected = allDropDowns[name].defaultOption;
		}
	}
	if ( auto highlightImageMid = dropdownFrame->findImage("interact selected highlight mid") )
	{
		highlightImageMid->disabled = true;
	}
	if ( auto highlightImageLeft = dropdownFrame->findImage("interact selected highlight left") )
	{
		highlightImageLeft->disabled = true;
	}
	if ( auto highlightImageRight = dropdownFrame->findImage("interact selected highlight right") )
	{
		highlightImageRight->disabled = true;
	}

	//Default reset. Otherwise will break mouse support after using gamepad once to trigger a context menu.
	dropDownToggleClick = false;

	if ( inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		player.hud.cursor.lastUpdateTick = ticks;
	}
}

void Player::GUIDropdown_t::create(const std::string name)
{
	if ( dropdownFrame )
	{
		dropdownFrame->removeSelf();
		dropdownFrame = nullptr;
	}
	if ( dropdownBlockClickFrame )
	{
		dropdownBlockClickFrame->removeSelf();
		dropdownBlockClickFrame = nullptr;
	}

	auto& dropDown = allDropDowns[name];

	char dropdownBlockClickName[64] = "";
	snprintf(dropdownBlockClickName, sizeof(dropdownBlockClickName), "player dropdown block click %d", player.playernum);
	dropdownBlockClickFrame = gameUIFrame[player.playernum]->addFrame(dropdownBlockClickName);
	dropdownBlockClickFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight()});
	dropdownBlockClickFrame->setOwner(player.playernum);

	char dropdownName[64] = "";
	snprintf(dropdownName, sizeof(dropdownName), "player dropdown %d", player.playernum);
	dropdownFrame = gameUIFrame[player.playernum]->addFrame(dropdownName);
	const int interactWidth = 106;
	dropdownFrame->setSize(SDL_Rect{ 0, 0, interactWidth + 6 * 2, 100 });
	dropdownFrame->setDisabled(true);
	dropdownFrame->setInheritParentFrameOpacity(false);

	Uint32 color = makeColor( 255, 255, 255, 255);
	const int topBackgroundHeight = 30;
	const int optionHeight = 20;

	dropdownFrame->addImage(SDL_Rect{ 24, 0, 0, 30 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_T03.png", "interact top background");
	dropdownFrame->addImage(SDL_Rect{ 0, 0, 24, 30 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_TL03.png", "interact top left");
	dropdownFrame->addImage(SDL_Rect{ 0, 0, 24, 30 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_TR03.png", "interact top right");

	dropdownFrame->addImage(SDL_Rect{ 24, 30, 0, 12 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_C03.png", "interact middle background");
	auto ml = dropdownFrame->addImage(SDL_Rect{ 0, 30, 24, 12 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_L03.png", "interact middle left");
	ml->tiled = true;
	auto mr = dropdownFrame->addImage(SDL_Rect{ 0, 30, 24, 12 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_R03.png", "interact middle right");
	mr->tiled = true;

	dropdownFrame->addImage(SDL_Rect{ 24, 96, 0, 14 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_B03.png", "interact bottom background");
	dropdownFrame->addImage(SDL_Rect{ 0, 96, 24, 14 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_BL03.png", "interact bottom left");
	dropdownFrame->addImage(SDL_Rect{ 0, 96, 24, 14 },
		color, "*#images/ui/Inventory/tooltips/HoverItemMenu_BR03.png", "interact bottom right");

	auto selectmid = dropdownFrame->addImage(SDL_Rect{ 6, optionHeight - 16, interactWidth, 22 },
		hudColors.itemContextMenuOptionSelectedImg, "*#images/ui/Inventory/tooltips/HoverItemMenu_SelectBack_M03.png", "interact selected highlight mid");
	selectmid->tiled = true;
	dropdownFrame->addImage(SDL_Rect{ 6, optionHeight - 16, 20, 22 },
		hudColors.itemContextMenuOptionSelectedImg, "*#images/ui/Inventory/tooltips/HoverItemMenu_SelectBack_L03.png", "interact selected highlight left");
	dropdownFrame->addImage(SDL_Rect{ 6, optionHeight - 16, 20, 22 },
		hudColors.itemContextMenuOptionSelectedImg, "*#images/ui/Inventory/tooltips/HoverItemMenu_SelectBack_R03.png", "interact selected highlight right");

	const char* interactFont = "fonts/pixel_maz.ttf#32#2";

	auto interactText = dropdownFrame->addField("interact text", 32);
	interactText->setText(dropDown.title.c_str());
	interactText->setSize(SDL_Rect{ 0, 2, 0, topBackgroundHeight });
	interactText->setFont(interactFont);
	interactText->setHJustify(Field::justify_t::CENTER);
	interactText->setVJustify(Field::justify_t::CENTER);
	interactText->setColor(hudColors.itemContextMenuHeadingText);

	const int interactOptionStartX = 4;
	const int interactOptionStartY = 33;
	const int glyphSize = 20;
	const int textWidth = 80;
	const int textHeight = glyphSize + 8;

	Uint32 textColor = hudColors.itemContextMenuOptionText;
	SDL_Rect prevGlyphPos;

	for ( int i = 1; i <= dropDown.options.size(); ++i )
	{
		char glyphname[32] = "";
		snprintf(glyphname, sizeof(glyphname), "glyph %d", i);
		Frame::image_t* interactGlyph = nullptr;
		if ( i == 1 )
		{
			interactGlyph = dropdownFrame->addImage(
				SDL_Rect{ interactOptionStartX + 4, interactOptionStartY + 4, glyphSize, glyphSize },
				0xFFFFFFFF, "", glyphname);
		}
		else if ( i > 1 )
		{
			interactGlyph = dropdownFrame->addImage(
				SDL_Rect{ interactOptionStartX + 4,
				prevGlyphPos.y + prevGlyphPos.h + 4,
				glyphSize, glyphSize },
				0xFFFFFFFF, "", glyphname);
		}

		const int textAlignX = interactGlyph->pos.x + interactGlyph->pos.w + 6;
		int textAlignY = interactGlyph->pos.y - 10;
		if ( i == 1 )
		{
			textAlignY = interactGlyph->pos.y - 8;
		}

		char optionname[32] = "";
		snprintf(optionname, sizeof(optionname), "interact option %d", i);
		interactText = dropdownFrame->addField(optionname, 32);
		interactText->setText("");
		interactText->setSize(SDL_Rect{
			textAlignX,
			textAlignY,
			textWidth, textHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::LEFT);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(textColor);

		prevGlyphPos = interactGlyph->pos;
	}
}

void Player::CharacterSheet_t::selectElement(SheetElements element, bool usingMouse, bool moveCursor)
{
	selectedElement = element;

	Frame* elementFrame = nullptr;
	Frame::image_t* img = nullptr;
	Field* elementField = nullptr;
	Button* elementButton = nullptr;
	bool selectedAButton = false;
	switch ( element )
	{
		case SHEET_OPEN_LOG:
			if ( elementFrame = sheetFrame->findFrame("log map buttons") )
			{
				if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
				{
					elementFrame->findButton("log button")->select();
					selectedAButton = true;
				}
				elementFrame = elementFrame->findFrame("log button selector");
				//img = elementFrame->findImage("log button img");
			}
			break;
		case SHEET_OPEN_MAP:
			if ( elementFrame = sheetFrame->findFrame("log map buttons") )
			{
				if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
				{
					elementFrame->findButton("map button")->select();
					selectedAButton = true;
				}
				elementFrame = elementFrame->findFrame("map button selector");
				//img = elementFrame->findImage("map button img");
			}
			break;
		case SHEET_SKILL_LIST:
			elementFrame = sheetFrame->findFrame("skills button frame");
			if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				elementFrame->findButton("skills button")->select();
				selectedAButton = true;
			}
			break;
		case SHEET_TIMER:
			elementFrame = sheetFrame->findFrame("game timer");
			if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				elementFrame->findButton("timer selector")->select();
				selectedAButton = true;
			}
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
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("str button");
				}
			}
			break;
		case SHEET_DEX:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("dex button");
				}
			}
			break;
		case SHEET_CON:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("con button");
				}
			}
			break;
		case SHEET_INT:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("int button");
				}
			}
			break;
		case SHEET_PER:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("per button");
				}
			}
			break;
		case SHEET_CHR:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("chr button");
				}
			}
			break;
		case SHEET_ATK:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("atk button");
				}
			}
			break;
		case SHEET_AC:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("ac button");
				}
			}
			break;
		case SHEET_POW:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("pow button");
				}
			}
			break;
		case SHEET_RES:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("res button");
				}
			}
			break;
		case SHEET_RGN:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("rgn button");
				}
			}
			break;
		case SHEET_RGN_MP:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("rgn mp button");
				}
			}
			break;
		case SHEET_WGT:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("wgt button");
				}
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
		if ( !isInteractable )
		{
			player.characterSheet.queuedElement = player.characterSheet.selectedElement;
		}
		else
		{
			player.characterSheet.queuedElement = SHEET_UNSELECTED;
			player.hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, usingMouse);
		}
	}
	if ( !selectedAButton && elementFrame && !inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		if ( elementButton )
		{
			elementButton->select();
		}
		else
		{
			elementFrame->select();
		}
	}
}

void Player::CharacterSheet_t::updateGameTimer()
{
	auto characterInfoFrame = sheetFrame->findFrame("character info");
	auto timerFrame = sheetFrame->findFrame("game timer");
	auto timerText = timerFrame->findField("timer text");
	char buf[32];

	Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
	Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
	Uint32 hour = (((completionTime / TICKS_PER_SECOND) / 60) / 60) % 24;
	Uint32 day = ((completionTime / TICKS_PER_SECOND) / 60) / 60 / 24;
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", day, hour, min, sec);
	timerText->setText(buf);

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick && inputs.getVirtualMouse(player.playernum)->draw_cursor;

	bool bCompactView = player.bUseCompactGUIHeight();
	if ( bCompactView )
	{
		enableTooltips = false;
	}

	if ( selectedElement == SHEET_TIMER && enableTooltips )
	{
		SDL_Rect tooltipPos = characterInfoFrame->getSize();
		tooltipPos.y = timerFrame->getSize().y;
		Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
		if ( panelJustify == PANEL_JUSTIFY_LEFT || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
		{
			tooltipJustify = PANEL_JUSTIFY_LEFT;
			tooltipPos.x += tooltipPos.w;
		}
		updateCharacterSheetTooltip(SHEET_TIMER, tooltipPos, tooltipJustify);
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

bool getAttackTooltipLines(int playernum, AttackHoverText_t& attackHoverTextInfo, int lineNumber, char titleBuf[128], char valueBuf[128])
{
	std::string skillName = "-";
	Sint32 skillLVL = 0;
	for ( auto& skill : Player::SkillSheet_t::skillSheetData.skillEntries )
	{
		if ( skill.skillId == attackHoverTextInfo.proficiency )
		{
			skillName = skill.name;
			skillLVL = stats[playernum]->PROFICIENCIES[attackHoverTextInfo.proficiency];
			break;
		}
	}

	if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MELEE_WEAPON )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(), 
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_melee").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_melee_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_MELEE_DAMAGE);
				return true;
			case 6:
				return false;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_UNARMED )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_melee").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_items_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.equipmentAndEffectBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_skill_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.proficiencyBonus);
				return true;
			case 6:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_unarmed_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_PLAYER_UNARMED_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_noskill").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_melee").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_melee_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_MELEE_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RANGED )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_ranged").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_items_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.equipmentAndEffectBonus);
				return true;
			case 6:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_ranged_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_RANGED_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN
		|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 2:
				snprintf(titleBuf, 128, "");
				snprintf(valueBuf, 128, "");
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_ranged").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_skill_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.proficiencyBonus);
				return true;
			case 6:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_thrown_weapon_fully_charged").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					3);
				return true;
			case 7:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_thrown_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_THROWN_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_POTION )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_skill_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.proficiencyBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_thrown_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_THROWN_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_WHIP )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_nobonus_format").c_str(),
					attackHoverTextInfo.totalAttack);
				return true;
			case 2:
				snprintf(titleBuf, 128, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_whip").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_melee_weapon_base").c_str());
				snprintf(valueBuf, 128,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_MELEE_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 128, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 128, "-");
				return true;
			case 2:
				snprintf(titleBuf, 128, "");
				snprintf(valueBuf, 128, "");
				return true;
			default:
				return false;
		}
	}
	return false;
}

real_t getDisplayedHPRegen(Entity* my, Stat& myStats, Uint32* outColor, char buf[32])
{
	real_t regen = 0.0;
	if ( outColor )
	{
		*outColor = hudColors.characterSheetNeutral;
	}
	if ( myStats.HP > 0 )
	{
		regen = (static_cast<real_t>(Entity::getHealthRegenInterval(my,
			myStats, true)) / TICKS_PER_SECOND);
		if ( myStats.type == SKELETON )
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
				if ( outColor )
				{
					*outColor = hudColors.characterSheetNeutral;
				}
			}
			else
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetRed;
				}
			}
		}
		else if ( regen < HEAL_TIME / TICKS_PER_SECOND )
		{
			if ( outColor )
			{
				*outColor = hudColors.characterSheetGreen;
			}
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
	if ( buf )
	{
		if ( !(svFlags & SV_FLAG_HUNGER) )
		{
			snprintf(buf, 32, "- ");
		}
		else
		{
			snprintf(buf, 32, "%.f%%", regen * 100.0);
		}
	}
	return regen * 100.0;
}

real_t getDisplayedMPRegen(Entity* my, Stat& myStats, Uint32* outColor, char buf[32])
{
	real_t regen = 0.0;
	bool isNegative = false;
	bool isInsectoid = false;
	if ( /*players[player.playernum]->entity*/ true )
	{
		regen = (static_cast<real_t>(Entity::getManaRegenInterval(my, myStats, true)) / TICKS_PER_SECOND);
		if ( myStats.type == AUTOMATON )
		{
			if ( myStats.HUNGER <= 300 )
			{
				isNegative = true;
				regen /= 6; // degrade faster
			}
			else if ( myStats.HUNGER > 1200 )
			{
				if ( myStats.MP / static_cast<real_t>(std::max(1, myStats.MAXMP)) <= 0.5 )
				{
					regen /= 4; // increase faster at < 50% mana
				}
				else
				{
					regen /= 2; // increase less faster at > 50% mana
				}
			}
			else if ( myStats.HUNGER > 300 )
			{
				// normal manaRegenInterval 300-1200 hunger.
			}
		}

		if ( regen < 0.0 /*stats[player]->playerRace == RACE_INSECTOID && stats[player]->appearance == 0*/ )
		{
			regen = 0.0;
		}

		if ( myStats.type == AUTOMATON )
		{
			if ( myStats.HUNGER <= 300 )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetRed;
				}
			}
			else if ( regen < static_cast<real_t>(getBaseManaRegen(my, myStats)) / TICKS_PER_SECOND )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetGreen;
				}
			}
		}
		else if ( myStats.playerRace == RACE_INSECTOID && myStats.appearance == 0 )
		{
			isInsectoid = true;
			if ( !(svFlags & SV_FLAG_HUNGER) )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetNeutral;
				}
			}
			else
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetRed;
				}
			}
		}
		else if ( regen < static_cast<real_t>(getBaseManaRegen(my, myStats)) / TICKS_PER_SECOND )
		{
			if ( outColor )
			{
				*outColor = hudColors.characterSheetGreen;
			}
		}
	}
	else
	{
		regen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
	}

	if ( isNegative )
	{
		regen *= -1; // negative
	}

	if ( isInsectoid )
	{
		real_t normalRegenTime = (1000.f * 30 * 1.5) / static_cast<float>(TICKS_PER_SECOND); // 30 base, insectoid does 1.5x in getHungerTickRate()
		normalRegenTime /= (std::max(myStats.MAXMP, 1)); // time for 1 mana in seconds
		normalRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

		regen = normalRegenTime / (regen * TICKS_PER_SECOND);
	}
	else
	{
		if ( regen > 0.01 || regen < -0.01 )
		{
			real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
			regen = nominalRegen / regen;
		}
	}
	if ( buf )
	{
		if ( myStats.type == AUTOMATON )
		{
			snprintf(buf, 32, Player::CharacterSheet_t::getHoverTextString("attributes_rgn_ht_small_format").c_str(), regen * 100.0);
		}
		else
		{
			snprintf(buf, 32, Player::CharacterSheet_t::getHoverTextString("attributes_rgn_small_format").c_str(), regen * 100.0);
		}
	}
	return regen * 100.0;
}

void Player::CharacterSheet_t::updateCharacterSheetTooltip(SheetElements element, SDL_Rect pos, Player::PanelJustify_t tooltipJustify)
{
	if ( !sheetFrame )
	{
		return;
	}
	auto tooltipFrame = sheetFrame->findFrame("sheet tooltip");
	if ( tooltipFrame->isDisabled() )
	{
		tooltipOpacitySetpoint = 0;
		tooltipOpacityAnimate = 1.0;
	}
	
	if ( static_cast<int>(tooltipFrame->getOpacity()) != tooltipOpacitySetpoint )
	{
		const real_t fpsScale = (144.f / std::max(1U, fpsLimit));
		if ( tooltipOpacitySetpoint == 0 )
		{
			if ( ticks - tooltipDeselectedTick > 5 )
			{
				real_t factor = 10.0;
				real_t setpointDiff = fpsScale * std::max(.05, (tooltipOpacityAnimate)) / (factor);
				tooltipOpacityAnimate -= setpointDiff;
				tooltipOpacityAnimate = std::max(0.0, tooltipOpacityAnimate);
			}
		}
		else
		{
			real_t setpointDiff = fpsScale * std::max(.05, (1.0 - tooltipOpacityAnimate)) / (1);
			tooltipOpacityAnimate += setpointDiff;
			tooltipOpacityAnimate = std::min(1.0, tooltipOpacityAnimate);
		}
		tooltipFrame->setOpacity(tooltipOpacityAnimate * 100);
	}
	else
	{
		tooltipFrame->setOpacity(tooltipOpacitySetpoint);
	}

	if ( element == SHEET_ENUM_END || element == SHEET_UNSELECTED
		|| player.GUI.activeModule != Player::GUI_t::MODULE_CHARACTERSHEET )
	{
		//tooltipFrame->setDisabled(true);
		return;
	}

	tooltipOpacitySetpoint = 0;
	tooltipOpacityAnimate = 1.0;
	tooltipFrame->setDisabled(false);
	tooltipFrame->setOpacity(100.0);
	tooltipDeselectedTick = ticks;

	Uint32 defaultColor = hudColors.characterSheetNeutral;
	auto txt = tooltipFrame->findField("tooltip text");
	txt->setColor(defaultColor);
	auto txtRightAlignHint = tooltipFrame->findField("tooltip text right align hint");
	txtRightAlignHint->setColor(hudColors.characterSheetFaintText);
	txtRightAlignHint->setDisabled(true);

	for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_TEXT_FIELDS; ++i )
	{
		char glyphName[32] = "";
		snprintf(glyphName, sizeof(glyphName), "glyph %d", i);
		if ( auto glyph = tooltipFrame->findImage(glyphName) )
		{
			glyph->disabled = true;
		}

		auto entry = characterSheetTooltipTextFields[player.playernum][i]; assert(entry);
		entry->setDisabled(true);
		entry->setHJustify(Frame::justify_t::LEFT);
		entry->setVJustify(Field::justify_t::CENTER);
		entry->setColor(defaultColor);
	}
	auto div = tooltipFrame->findImage("tooltip divider 1");
	div->disabled = true;
	auto div2 = tooltipFrame->findImage("tooltip divider 2");
	div2->disabled = true;

	for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++i )
	{
		auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][i];
		txtValueBackingFrame->setDisabled(true);
	}

	if ( !(element >= Player::CharacterSheet_t::SHEET_STR && element <= Player::CharacterSheet_t::SHEET_CHR) )
	{
		auto tooltipTopLeft = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_00.png";
		auto tooltipTop = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_00.png";
		imageSetWidthHeight9x9(tooltipFrame, skillsheetEffectBackgroundImages);
	}

	if ( element >= Player::CharacterSheet_t::SHEET_STR && element <= Player::CharacterSheet_t::SHEET_CHR )
	{
		auto tooltipTopLeft = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png";
		auto tooltipTop = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png";
		imageSetWidthHeight9x9(tooltipFrame, skillsheetEffectBackgroundImages);

		int maxWidth = 260;
		if ( getHoverTextString("stat_max_tooltip_width") != defaultString )
		{
			maxWidth = std::max(0, std::stoi(getHoverTextString("stat_max_tooltip_width")));
		}
		int minWidth = 0;
		if ( getHoverTextString("stat_min_tooltip_width") != defaultString )
		{
			minWidth = std::max(0, std::stoi(getHoverTextString("stat_min_tooltip_width")));
		}
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };

		std::string titleText = "";
		std::string descText = "";
		int value = 0;
		switch ( element )
		{
			case SHEET_STR:
				titleText = getHoverTextString("stat_str_title");
				descText = getHoverTextString("stat_str_desc");
				break;
			case SHEET_DEX:
				titleText = getHoverTextString("stat_dex_title");
				descText = getHoverTextString("stat_dex_desc");
				break;
			case SHEET_CON:
				titleText = getHoverTextString("stat_con_title");
				descText = getHoverTextString("stat_con_desc");
				break;
			case SHEET_INT:
				titleText = getHoverTextString("stat_int_title");
				descText = getHoverTextString("stat_int_desc");
				break;
			case SHEET_PER:
				titleText = getHoverTextString("stat_per_title");
				descText = getHoverTextString("stat_per_desc");
				break;
			case SHEET_CHR:
				titleText = getHoverTextString("stat_chr_title");
				descText = getHoverTextString("stat_chr_desc");
				break;
			default:
				break;
		}

		txt->setText(titleText.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1 - 2, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		txt->setColor(hudColors.characterSheetHeadingText);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txtPos.w = std::max(minWidth - padx * 2, txtPos.w);
		txt->setSize(txtPos);

		tooltipPos.w = (txtPos.w + padx * 2);

		unsigned int longestValue = 0;
		std::map<int, std::pair<Field*, SDL_Rect>> valueSizes;

		int currentHeight = txtPos.y + (actualFont->height(true) * 1) + 2;
		const int extraTextHeightForLowerCharacters = 4;
		{
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][1]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_base_amount").c_str());
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");
			glyphBacking->disabled = false;
			glyphBacking->path = getHoverTextString("icon_backing_path");
			glyphBacking->pos.x = padx + padxMid + 4;
			glyphBacking->pos.y = currentHeight + 6;
			glyphBacking->pos.w = 44;
			glyphBacking->pos.h = 44;

			auto glyphIcon = tooltipFrame->findImage("glyph 2");
			glyphIcon->disabled = false;
			glyphIcon->path = "";
			switch ( element )
			{
				case SHEET_STR:
					glyphIcon->path = getHoverTextString("icon_str_path");
					break;
				case SHEET_DEX:
					glyphIcon->path = getHoverTextString("icon_dex_path");
					break;
				case SHEET_CON:
					glyphIcon->path = getHoverTextString("icon_con_path");
					break;
				case SHEET_INT:
					glyphIcon->path = getHoverTextString("icon_int_path");
					break;
				case SHEET_PER:
					glyphIcon->path = getHoverTextString("icon_per_path");
					break;
				case SHEET_CHR:
					glyphIcon->path = getHoverTextString("icon_chr_path");
					break;
				default:
					break;
			}
			glyphIcon->pos.w = 24;
			glyphIcon->pos.h = 24;
			glyphIcon->pos.x = glyphBacking->pos.x + glyphBacking->pos.w / 2 - glyphIcon->pos.w / 2;
			glyphIcon->pos.y = glyphBacking->pos.y + glyphBacking->pos.h / 2 - glyphIcon->pos.h / 2;

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][2]; assert(entry);
			entryValue->setDisabled(false);
			int value = 0;
			char valueBuf[128] = "";
			switch ( element )
			{
				case SHEET_STR:
					value = stats[player.playernum]->STR;
					snprintf(valueBuf, sizeof(valueBuf), "%d STR", value);
					break;
				case SHEET_DEX:
					value = stats[player.playernum]->DEX;
					snprintf(valueBuf, sizeof(valueBuf), "%d DEX", value);
					break;
				case SHEET_CON:
					value = stats[player.playernum]->CON;
					snprintf(valueBuf, sizeof(valueBuf), "%d CON", value);
					break;
				case SHEET_INT:
					value = stats[player.playernum]->INT;
					snprintf(valueBuf, sizeof(valueBuf), "%d INT", value);
					break;
				case SHEET_PER:
					value = stats[player.playernum]->PER;
					snprintf(valueBuf, sizeof(valueBuf), "%d PER", value);
					break;
				case SHEET_CHR:
					value = stats[player.playernum]->CHR;
					snprintf(valueBuf, sizeof(valueBuf), "%d CHR", value);
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 1");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[1] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
		}
		{
			currentHeight += 0;// padyMid / 2;
			auto entry = characterSheetTooltipTextFields[player.playernum][3]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_modified_amount").c_str());
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][4]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
					value = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->STR;
					snprintf(valueBuf, sizeof(valueBuf), "%+d STR", value);
					break;
				case SHEET_DEX:
					value = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->DEX;
					snprintf(valueBuf, sizeof(valueBuf), "%+d DEX", value);
					break;
				case SHEET_CON:
					value = statGetCON(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->CON;
					snprintf(valueBuf, sizeof(valueBuf), "%+d CON", value);
					break;
				case SHEET_INT:
					value = statGetINT(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->INT;
					snprintf(valueBuf, sizeof(valueBuf), "%+d INT", value);
					break;
				case SHEET_PER:
					value = statGetPER(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->PER;
					snprintf(valueBuf, sizeof(valueBuf), "%+d PER", value);
					break;
				case SHEET_CHR:
					value = statGetCHR(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->CHR;
					snprintf(valueBuf, sizeof(valueBuf), "%+d CHR", value);
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 2");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[2] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
		}
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][5]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			if ( element == SHEET_STR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_str_atk_bonus").c_str());
			}
			else if ( element == SHEET_DEX )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_dex_ranged_atk_bonus").c_str());
			}
			else if ( element == SHEET_CON )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_con_ac_bonus").c_str());
			}
			else if ( element == SHEET_PER )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_per_pierce_bonus").c_str());
			}
			else if ( element == SHEET_INT )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_int_pwr_bonus").c_str());
			}
			else if ( element == SHEET_CHR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_chr_buying_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][6]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
				{
					Sint32 STR = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_atk_value_format").c_str(), STR);
				}
					break;
				case SHEET_DEX:
				{
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_atk_value_format").c_str(), DEX);
				}
					break;
				case SHEET_CON:
				{
					Sint32 CON = statGetCON(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_ac_value_format").c_str(), CON);
				}
					break;
				case SHEET_INT:
				{
					real_t val = getBonusFromCasterOfSpellElement(players[player.playernum]->entity, stats[player.playernum]) * 100.0;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pwr_value_format").c_str(), val);
				}
					break;
				case SHEET_PER:
				{
					real_t val = std::min(std::max(statGetPER(stats[player.playernum], players[player.playernum]->entity) / 2, 0), 50);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pierce_value_format").c_str(), val);
				}
					break;
				case SHEET_CHR:
				{
					real_t val = 1 / ((50 + stats[player.playernum]->PROFICIENCIES[PRO_TRADING]) / 150.f); // buy value
					real_t normalVal = val;
					normalVal /= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					normalVal = std::max(1.0, normalVal);

					int stat = stats[player.playernum]->CHR;
					stats[player.playernum]->CHR = 0;
					stats[player.playernum]->CHR -= statGetCHR(stats[player.playernum], players[player.playernum]->entity);
					real_t zeroVal = val;
					zeroVal /= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					zeroVal = std::max(1.0, zeroVal);
					stats[player.playernum]->CHR = stat;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_buying_value_format").c_str(), (zeroVal - normalVal) * 100.0);
				}
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][3];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[3] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}
		if ( element == SHEET_STR || element == SHEET_DEX || element == SHEET_INT || element == SHEET_PER || element == SHEET_CHR )
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][7]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";

			if ( element == SHEET_STR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_str_movement_bonus").c_str());
			}
			else if ( element == SHEET_DEX )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_dex_thrown_atk_bonus").c_str());
			}
			else if ( element == SHEET_INT )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_int_mp_regen_bonus").c_str());
			}
			else if ( element == SHEET_PER )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_per_light_bonus").c_str());
			}
			else if ( element == SHEET_CHR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_chr_selling_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][8]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
				{
					Sint32 STR = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					real_t weightratio1 = player.movement.getWeightRatio(player.movement.getCharacterModifiedWeight(), 0);
					real_t weightratio2 = player.movement.getWeightRatio(player.movement.getCharacterModifiedWeight(), STR);
					real_t speedFactor1 = player.movement.getSpeedFactor(weightratio1, DEX);
					real_t speedFactor2 = player.movement.getSpeedFactor(weightratio2, DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t noSTRPercent = 100.0 * speedFactor1 / std::fmax(.01, maxSpeed);
					real_t currentPercent = 100.0 * speedFactor2 / std::fmax(.01, maxSpeed);
					real_t displayValue = currentPercent - noSTRPercent;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_movement_value_format").c_str(), displayValue);
				}
					break;
				case SHEET_DEX:
				{
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_atk_value_format").c_str(), DEX / 4);
				}
					break;
				case SHEET_CON:
					break;
				case SHEET_INT:
				{
					Sint32 oldINT = stats[player.playernum]->INT;
					stats[player.playernum]->INT += -statGetINT(stats[player.playernum], player.entity);
					real_t regenWithoutINT = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
					stats[player.playernum]->INT = oldINT;

					real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
					real_t regenStatSkill = regenTotal - regenWithoutINT;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_mp_regen_value_format").c_str(), regenStatSkill);
				}
					break;
				case SHEET_PER:
				{
					Sint32 PER = statGetPER(stats[player.playernum], players[player.playernum]->entity);
					int lightBonus = PER / 3;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_light_value_format").c_str(), lightBonus);
				}
					break;
				case SHEET_CHR:
				{
					real_t val = (50 + stats[player.playernum]->PROFICIENCIES[PRO_TRADING]) / 150.f; // sell value
					real_t normalVal = val;
					normalVal *= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					normalVal = std::min(1.0, normalVal);

					int stat = stats[player.playernum]->CHR;
					stats[player.playernum]->CHR = 0;
					stats[player.playernum]->CHR -= statGetCHR(stats[player.playernum], players[player.playernum]->entity);
					real_t zeroVal = val;
					zeroVal *= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					zeroVal = std::min(1.0, zeroVal);
					stats[player.playernum]->CHR = stat;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_selling_value_format").c_str(), (normalVal - zeroVal) * 100.0);
				}
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][4];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[4] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}
		if ( element == SHEET_PER || element == SHEET_DEX )
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][9]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			if ( element == SHEET_DEX )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_dex_movement_bonus").c_str());
			}
			else if ( element == SHEET_PER )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_per_sneaking_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][10]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
					break;
				case SHEET_DEX:
				{
					Sint32 STR = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					real_t weightratio = player.movement.getWeightRatio(player.movement.getCharacterModifiedWeight(), STR);
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					real_t speedFactor1 = player.movement.getSpeedFactor(weightratio, 0);
					real_t speedFactor2 = player.movement.getSpeedFactor(weightratio, DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t noDEXPercent = 100.0 * speedFactor1 / std::fmax(.01, maxSpeed);
					real_t currentPercent = 100.0 * speedFactor2 / std::fmax(.01, maxSpeed);

					real_t displayValue = currentPercent - noDEXPercent;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_movement_value_format").c_str(), displayValue);
				}
					break;
				case SHEET_CON:
					break;
				case SHEET_INT:
					break;
				case SHEET_PER:
				{
					int sneakingBonus = (2 + (stats[player.playernum]->PROFICIENCIES[PRO_STEALTH] / 40));
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_light_value_format").c_str(), sneakingBonus);
				}
				break;
				case SHEET_CHR:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][5];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[5] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}

		for ( int index = 1; index <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++index )
		{
			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][index];
			if ( txtValueBackingFrame->isDisabled() )
			{
				continue;
			}
			if ( valueSizes.find(index) == valueSizes.end() )
			{
				continue;
			}

			SDL_Rect valuePos = valueSizes[index].second;
			Field* entryValue = valueSizes[index].first;
			SDL_Rect entryValuePos = entryValue->getSize();
			entryValuePos.x = entryValuePos.x + entryValuePos.w;
			entryValuePos.w = (int)longestValue;
			entryValuePos.x -= entryValuePos.w;
			entryValuePos.x -= 8;
			entryValue->setSize(entryValuePos);

			valuePos.w = (int)longestValue + 16;
			valuePos.x -= (valuePos.w);
			valuePos.y -= 3;
			valuePos.h += 4;

			txtValueBackingFrame->setSize(valuePos);

			imageResizeToContainer9x9(txtValueBackingFrame, SDL_Rect{ 0, 0, valuePos.w, valuePos.h }, skillsheetEffectBackgroundImages);
		}

		{
			currentHeight += padyMid;

			div->pos.x = padx;
			div->pos.y = currentHeight;
			div->pos.w = txtPos.w;
			div->disabled = false;

			currentHeight += padyMid;

			auto entry = characterSheetTooltipTextFields[player.playernum][11]; assert(entry);
			entry->setDisabled(false);
			char buf[512] = "";

			std::string descTextFormatted = "\x1E ";
			for ( auto s : descText )
			{
				descTextFormatted += s;
				if ( s == '\n' )
				{
					descTextFormatted += "\x1E ";
				}
			}

			snprintf(buf, sizeof(buf), "%s", descTextFormatted.c_str());
			entry->setText(buf);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w;
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(hudColors.characterSheetOffWhiteText);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);

			currentHeight += padyMid / 4;
			tooltipPos.h = pady1 + currentHeight + pady2;
		}

		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			skillsheetEffectBackgroundImages);
	}
	else if ( element >= Player::CharacterSheet_t::SHEET_ATK && element <= Player::CharacterSheet_t::SHEET_WGT )
	{
		AttackHoverText_t attackHoverTextInfo;
		Sint32 attackPower = displayAttackPower(player.playernum, attackHoverTextInfo);

//#ifndef NDEBUG
//		if ( keystatus[SDL_SCANCODE_V] )
//		{
//			keystatus[SDL_SCANCODE_V] = 0;
//			messagePlayer(player.playernum, MESSAGE_DEBUG, "Remove this");
//			stats[player.playernum]->playerRace = RACE_AUTOMATON;
//			stats[player.playernum]->appearance = 0;
//		}
//		if ( keystatus[SDL_SCANCODE_B] )
//		{
//			keystatus[SDL_SCANCODE_B] = 0;
//			messagePlayer(player.playernum, MESSAGE_DEBUG, "Remove this");
//			stats[player.playernum]->playerRace = RACE_INSECTOID;
//			stats[player.playernum]->appearance = 0;
//		}
//#endif // !NDEBUG

		bool isAutomatonHTRegen = stats[player.playernum]->type == AUTOMATON;
		bool isInsectoidENRegen = (stats[player.playernum]->playerRace == RACE_INSECTOID && stats[player.playernum]->appearance == 0);

		auto tooltipTopLeft = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png";
		auto tooltipTop = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png";
		imageSetWidthHeight9x9(tooltipFrame, skillsheetEffectBackgroundImages);

		int maxWidth = 260;
		if ( getHoverTextString("attributes_max_tooltip_width") != defaultString )
		{
			maxWidth = std::max(0, std::stoi(getHoverTextString("attributes_max_tooltip_width")));
		}
		int minWidth = 0;
		if ( getHoverTextString("attributes_min_tooltip_width") != defaultString )
		{
			minWidth = std::max(0, std::stoi(getHoverTextString("attributes_min_tooltip_width")));
		}
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };

		std::string titleText = "";
		std::string descText = "";
		int value = 0;
		switch ( element )
		{
			case SHEET_ATK:
			{
				char descBuf[256];
				std::string skillName = "-";
				for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
				{
					if ( skill.skillId == attackHoverTextInfo.proficiency )
					{
						skillName = skill.name;
						break;
					}
				}
				titleText = getHoverTextString("attributes_atk_title");
				txtRightAlignHint->setText("");
				txtRightAlignHint->setDisabled(false);
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_UNARMED )
				{
					txtRightAlignHint->setText(getHoverTextString("attributes_atk_title_unarmed").c_str());
				}
				else if ( stats[player.playernum]->weapon )
				{
					Item* item = stats[player.playernum]->weapon;
					if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
					{
						char itemNameBuf[128];
						if ( itemCategory(item) == MAGICSTAFF )
						{
							snprintf(itemNameBuf, sizeof(itemNameBuf), "%s (%+d)", item->getName(), item->beatitude);
							std::string itemNameStr = itemNameBuf;
							capitalizeString(itemNameStr);
							txtRightAlignHint->setText(itemNameStr.c_str());
						}
						else
						{
							snprintf(itemNameBuf, sizeof(itemNameBuf), "%s %s (%+d)", 
								ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
							txtRightAlignHint->setText(itemNameBuf);
						}
					}
				}
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MELEE_WEAPON
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_WHIP )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_melee_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_UNARMED )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_unarmed_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RANGED )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_ranged_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_thrown_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_gem_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_POTION )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_potion_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_pickaxe_desc").c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_magicstaff_desc").c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_tool_desc").c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_tinker_tool_desc").c_str());
					descText = descBuf;
				}
				else
				{
					descText = "";
				}
			}
				break;
			case SHEET_AC:
				titleText = getHoverTextString("attributes_ac_title");
				descText = getHoverTextString("attributes_ac_desc");
				break;
			case SHEET_POW:
				titleText = getHoverTextString("attributes_pwr_title");
				descText = getHoverTextString("attributes_pwr_desc");
				break;
			case SHEET_RES:
				titleText = getHoverTextString("attributes_res_title");
				descText = getHoverTextString("attributes_res_desc");
				break;
			case SHEET_RGN:
				titleText = getHoverTextString("attributes_rgn_hp_title");
				if ( !(svFlags & SV_FLAG_HUNGER) )
				{
					descText = getHoverTextString("attributes_rgn_hp_desc_no_hunger");
				}
				else
				{
					descText = getHoverTextString("attributes_rgn_hp_desc");
				}
				break;
			case SHEET_RGN_MP:
				if ( isAutomatonHTRegen )
				{
					titleText = getHoverTextString("attributes_rgn_ht_title");
					descText = getHoverTextString("attributes_rgn_ht_desc");
				}
				else if ( isInsectoidENRegen )
				{
					titleText = getHoverTextString("attributes_rgn_en_title");
					descText = getHoverTextString("attributes_rgn_en_desc");
				}
				else
				{
					titleText = getHoverTextString("attributes_rgn_mp_title");
					descText = getHoverTextString("attributes_rgn_mp_desc");
				}
				break;
			case SHEET_WGT:
				titleText = getHoverTextString("attributes_wgt_title");
				descText = getHoverTextString("attributes_wgt_desc");
				break;
			default:
				break;
		}

		txt->setText(titleText.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1 - 2, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		txt->setColor(hudColors.characterSheetHeadingText);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txtPos.w = std::max(minWidth - padx * 2, txtPos.w);
		txt->setSize(txtPos);

		txtRightAlignHint->setSize(txtPos);
		txtRightAlignHint->setHJustify(Field::justify_t::RIGHT);

		tooltipPos.w = (txtPos.w + padx * 2);

		unsigned int longestValue = 0;
		std::map<int, std::pair<Field*, SDL_Rect>> valueSizes;

		int currentHeight = txtPos.y + (actualFont->height(true) * 1) + 2;
		const int extraTextHeightForLowerCharacters = 4;
		int currentTextFieldIndex = 1;
		int currentTextBackingFrameIndex = 1;

		char buf[128] = "";
		char valueBuf[128] = "";

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 1, buf, valueBuf)
			|| element != SHEET_ATK )
		{
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_base").c_str());
					bool oldDefending = stats[player.playernum]->defending;
					stats[player.playernum]->defending = false;
					Sint32 armor = AC(stats[player.playernum]);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_nobonus_format").c_str(), armor);
					stats[player.playernum]->defending = oldDefending;
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_base").c_str());
					std::string tag = "MAGIC_SPELLPOWER";
					std::string formatValue = "%d";
					std::string pwrBonus = formatSkillSheetEffects(player.playernum, PRO_MAGIC, tag, formatValue);
					Sint32 pwr = 100 + std::stoi(pwrBonus);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_nobonus_format").c_str(), pwr);
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_base").c_str());
					real_t resistance = 100.0 * hit.entity->getDamageTableMultiplier(*stats[player.playernum], DAMAGE_TABLE_MAGIC);
					resistance /= (Entity::getMagicResistance(stats[player.playernum]) + 1);
					resistance = 100.0 - resistance;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_nobonus_format").c_str(), (int)resistance);
				}
					break;
				case SHEET_RGN:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_hp_base").c_str());
					char hpbuf[32] = "";
					getDisplayedHPRegen(players[player.playernum]->entity, *stats[player.playernum], nullptr, hpbuf);
					if ( !(svFlags & SV_FLAG_HUNGER) )
					{
						snprintf(valueBuf, sizeof(valueBuf), "%s", hpbuf);
					}
					else
					{
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_nobonus_format").c_str(), hpbuf);
					}
				}
					break;
				case SHEET_RGN_MP:
				{
					char mpbuf[32] = "";
					real_t regen = getDisplayedMPRegen(players[player.playernum]->entity, *stats[player.playernum], nullptr, mpbuf);
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_ht_base").c_str());
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_nobonus_format").c_str(), mpbuf);
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_base").c_str());
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_nobonus_format").c_str(), mpbuf);
					}
					else
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_mp_base").c_str());
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_nobonus_format").c_str(), mpbuf);
					}
				}
					break;
				case SHEET_WGT:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_base").c_str());
					int weight = player.movement.getCharacterWeight();
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_nobonus_format").c_str(), weight);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");
			glyphBacking->disabled = false;
			glyphBacking->path = getHoverTextString("icon_backing_path");
			glyphBacking->pos.x = padx + padxMid + 4;
			glyphBacking->pos.y = currentHeight + 6;
			glyphBacking->pos.w = 44;
			glyphBacking->pos.h = 44;

			auto glyphIcon = tooltipFrame->findImage("glyph 2");
			glyphIcon->disabled = false;
			glyphIcon->path = "";
			switch ( element )
			{
				case SHEET_ATK:
					if ( attackHoverTextInfo.proficiency == -1 )
					{
						glyphIcon->path = getHoverTextString("icon_atk_path");
					}
					else if (attackHoverTextInfo.proficiency >= 0 && attackHoverTextInfo.proficiency < NUMPROFICIENCIES )
					{
						for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
						{
							if ( skill.skillId == attackHoverTextInfo.proficiency )
							{
								if ( skillCapstoneUnlocked(player.playernum, attackHoverTextInfo.proficiency) )
								{
									glyphIcon->path = skill.skillIconPathLegend;
								}
								else
								{
									glyphIcon->path = skill.skillIconPath;
								}
								break;
							}
						}
						if ( stats[player.playernum]->PROFICIENCIES[attackHoverTextInfo.proficiency] >= SKILL_LEVEL_LEGENDARY )
						{
							glyphBacking->path = actionPromptBackingIconPath100;
						}
						else if ( stats[player.playernum]->PROFICIENCIES[attackHoverTextInfo.proficiency] >= SKILL_LEVEL_EXPERT )
						{
							glyphBacking->path = actionPromptBackingIconPath60;
						}
						else if ( stats[player.playernum]->PROFICIENCIES[attackHoverTextInfo.proficiency] >= SKILL_LEVEL_BASIC )
						{
							glyphBacking->path = actionPromptBackingIconPath20;
						}
						else
						{
							glyphBacking->path = actionPromptBackingIconPath00;
						}
					}
					break;
				case SHEET_AC:
					glyphIcon->path = getHoverTextString("icon_ac_path");
					for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
					{
						if ( skill.skillId == PRO_SHIELD )
						{
							if ( skillCapstoneUnlocked(player.playernum, PRO_SHIELD) )
							{
								glyphIcon->path = skill.skillIconPathLegend;
							}
							else
							{
								glyphIcon->path = skill.skillIconPath;
							}
							break;
						}
					}
					if ( stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] >= SKILL_LEVEL_LEGENDARY )
					{
						glyphBacking->path = actionPromptBackingIconPath100;
					}
					else if ( stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] >= SKILL_LEVEL_EXPERT )
					{
						glyphBacking->path = actionPromptBackingIconPath60;
					}
					else if ( stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] >= SKILL_LEVEL_BASIC )
					{
						glyphBacking->path = actionPromptBackingIconPath20;
					}
					else
					{
						glyphBacking->path = actionPromptBackingIconPath00;
					}
					break;
				case SHEET_POW:
					glyphIcon->path = getHoverTextString("icon_pwr_path");
					break;
				case SHEET_RES:
					glyphIcon->path = getHoverTextString("icon_res_path");
					break;
				case SHEET_RGN:
					glyphIcon->path = getHoverTextString("icon_rgn_hp_path");
					break;
				case SHEET_RGN_MP:
					if ( isAutomatonHTRegen )
					{
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							glyphIcon->path = getHoverTextString("icon_rgn_ht_empty_path");
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							glyphIcon->path = getHoverTextString("icon_rgn_ht_superheated_path");
						}
						else
						{
							glyphIcon->path = getHoverTextString("icon_rgn_ht_normal_path");
						}
					}
					else
					{
						glyphIcon->path = getHoverTextString("icon_rgn_mp_path");
					}
					break;
				case SHEET_WGT:
					glyphIcon->path = getHoverTextString("icon_wgt_path");
					break;
				default:
					break;
			}
			glyphIcon->pos.w = 24;
			glyphIcon->pos.h = 24;
			glyphIcon->pos.x = glyphBacking->pos.x + glyphBacking->pos.w / 2 - glyphIcon->pos.w / 2;
			glyphIcon->pos.y = glyphBacking->pos.y + glyphBacking->pos.h / 2 - glyphIcon->pos.h / 2;

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			if ( element == SHEET_ATK )
			{
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE )
				{
					// fewer lines, add offset to centre the lines with the glyph
					entryPos.y += 8;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_DEFAULT
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF )
				{
					entryPos.y += 16;
				}
			}
			else if ( element == SHEET_RGN_MP && isInsectoidENRegen )
			{
				entryPos.y += 16;
			}
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 1");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[1] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
			++currentTextBackingFrameIndex;
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 2, buf, valueBuf)
			|| element != SHEET_ATK )
		{
			currentHeight += 0;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					std::string skillName = "";
					int skillLVL = 0;
					for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
					{
						if ( skill.skillId == PRO_SHIELD )
						{
							skillName = skill.name;
							skillLVL = stats[player.playernum]->PROFICIENCIES[skill.skillId];
							break;
						}
					}
					snprintf(buf, sizeof(buf), getHoverTextString("attributes_ac_defending").c_str(), skillName.c_str(), skillLVL);
					std::string tag = "BLOCK_AC_INCREASE";
					std::string blockBonus = formatSkillSheetEffects(player.playernum, PRO_SHIELD, tag, getHoverTextString("attributes_ac_bonus_format"));
					snprintf(valueBuf, sizeof(valueBuf), "%s", blockBonus.c_str());
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_spellbook").c_str());
					std::string tag = "MAGIC_SPELLPOWER";
					std::string formatValue = "%d";
					std::string pwrBonus = formatSkillSheetEffects(player.playernum, PRO_MAGIC, tag, formatValue);
					Sint32 pwr = std::stoi(pwrBonus) / 2;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_bonus_format").c_str(), pwr);
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_sources").c_str());
					int sources = Entity::getMagicResistance(stats[player.playernum]);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_sources_format").c_str(), sources);
				}
					break;
				case SHEET_RGN:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_sources").c_str());
					int sources = Entity::getHealringFromEquipment(players[player.playernum]->entity, *stats[player.playernum], true);
					sources += Entity::getHealringFromEffects(players[player.playernum]->entity, *stats[player.playernum]);
					sources = std::min(sources, 3);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_hp_sources_format").c_str(), sources);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isInsectoidENRegen )
					{
						//snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_sources").c_str());
						snprintf(buf, sizeof(buf), "");
						snprintf(valueBuf, sizeof(valueBuf), "");
					}
					else
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_sources").c_str());
						int sources = Entity::getManaringFromEquipment(players[player.playernum]->entity, *stats[player.playernum], true);
						sources += Entity::getManaringFromEffects(players[player.playernum]->entity, *stats[player.playernum]);
						sources = std::min(sources, 3);
						if ( isAutomatonHTRegen )
						{
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_sources_format").c_str(), sources);
						}
						else
						{
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_mp_sources_format").c_str(), sources);
						}
					}
				}
					break;
				case SHEET_WGT:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_movement_speed").c_str());
					int weight = player.movement.getCharacterModifiedWeight();
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_format").c_str(), 
						100.0 * currentSpeed / std::fmax(.01, maxSpeed));
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);

			if ( strcmp(buf, "") )
			{
				// don't modify height if this is an empty line
				currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			}
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 2");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[2] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
			++currentTextBackingFrameIndex;
		}
		bool hasEntryInfoLines = false;
		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 3, buf, valueBuf)
			|| (element != SHEET_ATK && !(element == SHEET_RGN && !(svFlags & SV_FLAG_HUNGER))) )
		{
			// extra number display - line 3
			hasEntryInfoLines = true;
			if ( element == SHEET_ATK || element == SHEET_AC || element == SHEET_POW || element == SHEET_RES || element == SHEET_RGN
				|| element == SHEET_RGN_MP || element == SHEET_WGT )
			{
				if ( element == SHEET_RGN_MP && isInsectoidENRegen )
				{
					currentHeight += 8;
				}

				// add a divider
				div2->pos.x = padx;
				div2->pos.y = currentHeight + 2 + actualFont->height(true);
				div2->pos.w = txtPos.w;
				div2->disabled = false;

				auto entryTotalHeading = characterSheetTooltipTextFields[player.playernum][16]; assert(entryTotalHeading);
				entryTotalHeading->setDisabled(false);
				if ( element == SHEET_ATK )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_atk_total_sum_header").c_str());
				}
				else if ( element == SHEET_AC )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_ac_base_sum_header").c_str());
				}
				else if ( element == SHEET_POW )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_pwr_base_sum_header").c_str());
				}
				else if ( element == SHEET_RES )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_res_base_sum_header").c_str());
				}
				else if ( element == SHEET_RGN )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_rgn_hp_base_sum_header").c_str());
				}
				else if ( element == SHEET_RGN_MP )
				{
					if ( isAutomatonHTRegen )
					{
						entryTotalHeading->setText(getHoverTextString("attributes_rgn_ht_base_sum_header").c_str());
					}
					else if ( isInsectoidENRegen )
					{
						entryTotalHeading->setText(getHoverTextString("attributes_rgn_en_base_sum_header").c_str());
					}
					else
					{
						entryTotalHeading->setText(getHoverTextString("attributes_rgn_mp_base_sum_header").c_str());
					}
				}
				else if ( element == SHEET_WGT )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_wgt_base_sum_header").c_str());
				}
				entryTotalHeading->setColor(hudColors.characterSheetOffWhiteText);
				entryTotalHeading->setHJustify(Field::justify_t::RIGHT);
				SDL_Rect entryPos = entryTotalHeading->getSize();
				entryPos.x = padx + padxMid;
				entryPos.y = currentHeight + 1 - (extraTextHeightForLowerCharacters / 2);
				entryPos.w = txtPos.w - (padxMid * 2);
				entryPos.h = actualFont->height(true) + extraTextHeightForLowerCharacters;
				entryTotalHeading->setSize(entryPos);

				currentHeight += actualFont->height(true) + 2; // extra gap here for 'total' text.
			}
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_entry_attr_bonus").c_str());
					Sint32 CON = statGetCON(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_bonus_format").c_str(), CON);
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_base_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_bonus_format").c_str(), 100);
				}
					break;
				case SHEET_RES:
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

					snprintf(buf, sizeof(buf), getHoverTextString("attributes_res_base_value").c_str(), race.c_str());
					Sint32 baseResist = damagetables[stats[player.playernum]->type][DAMAGE_TABLE_MAGIC] * 100;
					baseResist = 100 - baseResist;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_bonus_format").c_str(), baseResist);
				}
					break;
				case SHEET_RGN:
				{
					Monster type = stats[player.playernum]->type;
					bool aestheticOnly = false;
					if ( player.entity )
					{
						if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
						{
							if ( stats[player.playernum]->appearance != 0 )
							{
								aestheticOnly = true;
								type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
							}
						}
					}
					std::string race = getMonsterLocalizedName(type).c_str();
					capitalizeString(race);

					snprintf(buf, sizeof(buf), getHoverTextString("attributes_rgn_base_value").c_str(), race.c_str());
					real_t regen = 100.0;
					if ( type == SKELETON )
					{
						regen = 25.0;
					}
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regen);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_ht_base_bonus").c_str());
						real_t baseHTModifier = 100.f;
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							int baseTime = getBaseManaRegen(player.entity, *stats[player.playernum]);
							real_t scaledInterval = ((60 * baseTime) / (std::max(stats[player.playernum]->MAXMP, 1)));
							baseHTModifier = scaledInterval / TICKS_PER_SECOND;
							baseHTModifier /= -6.0; // degrade faster
							real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
							baseHTModifier = (nominalRegen / baseHTModifier) * 100.0;
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							if ( stats[player.playernum]->MP / static_cast<real_t>(std::max(1, stats[player.playernum]->MAXMP)) <= 0.5 )
							{
								baseHTModifier *= 4; // increase faster at < 50% mana
							}
							else
							{
								baseHTModifier *= 2; // increase less faster at > 50% mana
							}
						}
						else
						{
							// normal manaRegenInterval 300-1200 hunger.
						}
						//baseHTModifier /= 100.0;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_bonus_format").c_str(), baseHTModifier);
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_base_bonus").c_str());

						real_t normalRegenTime = (1000.f * 30 * 1.5) / static_cast<float>(TICKS_PER_SECOND); // 30 base, insectoid does 1.5x in getHungerTickRate()
						//normalRegenTime = (1000.f * (Entity::getHungerTickRate(stats[player.playernum], true, true)) / static_cast<float>(TICKS_PER_SECOND));
						normalRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						normalRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t modifiedRegenTime = (1000.f * (Entity::getHungerTickRate(stats[player.playernum], true, false)) / static_cast<float>(TICKS_PER_SECOND));
						modifiedRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						modifiedRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t displayValue = 100.0 * (normalRegenTime / modifiedRegenTime);

						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_bonus_format").c_str(), displayValue);
					}
					else
					{
						Monster type = stats[player.playernum]->type;
						bool aestheticOnly = false;
						if ( player.entity )
						{
							if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
							{
								if ( stats[player.playernum]->appearance != 0 )
								{
									aestheticOnly = true;
									type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
								}
							}
						}
						std::string race = getMonsterLocalizedName(type).c_str();
						capitalizeString(race);

						snprintf(buf, sizeof(buf), getHoverTextString("attributes_rgn_base_value").c_str(), race.c_str());
						real_t regen = 100.0;
						if ( type == SKELETON )
						{
							regen = 25.0;
						}
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regen);
					}
				}
				break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					//int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					//int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					//int goldWeightTotal = stats[player.playernum]->GOLD / 100;
					//int goldWeight = player.movement.getCharacterModifiedWeight(&goldWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					//real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					//real_t goldSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(goldWeight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					//real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);
					//real_t goldSpeedPercent = 100.0 * goldSpeed / std::fmax(.01, maxSpeed);

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_attributes_bonus").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						currentSpeedPercent + (noWeightSpeedPercent - currentSpeedPercent) /*+ (goldSpeedPercent - noWeightSpeedPercent)*/);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
			++currentTextBackingFrameIndex;
		}

		if ( (element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 4, buf, valueBuf))
			|| (element != SHEET_ATK && !(element == SHEET_RGN && !(svFlags & SV_FLAG_HUNGER))) )
		{
			// extra number display - line 4
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_entry_items_bonus").c_str());
					Sint32 CON = statGetCON(stats[player.playernum], players[player.playernum]->entity);

					Sint32 oldSkillLVL = stats[player.playernum]->PROFICIENCIES[PRO_SHIELD];
					bool oldDefending = stats[player.playernum]->defending;
					stats[player.playernum]->defending = false;
					stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] = 0;

					Sint32 armor = AC(stats[player.playernum]);
					stats[player.playernum]->defending = oldDefending;
					stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] = oldSkillLVL;

					Sint32 itemsEffectBonus = armor - CON;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_bonus_format").c_str(), itemsEffectBonus);
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_entry_attr_bonus").c_str());
					std::string tag = "MAGIC_SPELLPOWER";
					std::string pwrINTBonus = formatSkillSheetEffects(player.playernum, PRO_MAGIC, tag, getHoverTextString("attributes_pwr_bonus_format"));
					snprintf(valueBuf, sizeof(valueBuf), "%s", pwrINTBonus.c_str());
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_entry_items_bonus").c_str());
					Sint32 baseResist = 100 * damagetables[stats[player.playernum]->type][DAMAGE_TABLE_MAGIC];
					baseResist = 100 - baseResist;
					real_t resistance = 100.0 * hit.entity->getDamageTableMultiplier(*stats[player.playernum], DAMAGE_TABLE_MAGIC);
					resistance /= (Entity::getMagicResistance(stats[player.playernum]) + 1);
					resistance = (100.0 - resistance);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_bonus_format").c_str(), (int)resistance - baseResist);
				}
					break;
				case SHEET_RGN:
				{
					Monster type = stats[player.playernum]->type;
					bool aestheticOnly = false;
					if ( player.entity )
					{
						if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
						{
							if ( stats[player.playernum]->appearance != 0 )
							{
								aestheticOnly = true;
								type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
							}
						}
					}
					real_t baseRegen = 100.0;
					if ( type == SKELETON )
					{
						baseRegen = 25.0;
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_items_bonus").c_str());
					real_t regen = getDisplayedHPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regen - baseRegen);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_items_bonus").c_str());
						real_t baseHTModifier = 100.f;
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							int baseTime = getBaseManaRegen(player.entity, *stats[player.playernum]);
							real_t scaledInterval = ((60 * baseTime) / (std::max(stats[player.playernum]->MAXMP, 1)));
							baseHTModifier = scaledInterval / TICKS_PER_SECOND;
							baseHTModifier /= -6.0; // degrade faster
							real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
							baseHTModifier = (nominalRegen / baseHTModifier) * 100.0;
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							if ( stats[player.playernum]->MP / static_cast<real_t>(std::max(1, stats[player.playernum]->MAXMP)) <= 0.5 )
							{
								baseHTModifier *= 4; // increase faster at < 50% mana
							}
							else
							{
								baseHTModifier *= 2; // increase less faster at > 50% mana
							}
						}
						else
						{
							// normal manaRegenInterval 300-1200 hunger.
						}
						real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						real_t displayTotal = (regenTotal - baseHTModifier);
						//displayTotal /= 100.0;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_bonus_format").c_str(), displayTotal);
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_entry_items_bonus").c_str());

						real_t normalRegenTime = (1000.f * 30 * 1.5) / static_cast<float>(TICKS_PER_SECOND); // 30 base, insectoid does 1.5x in getHungerTickRate()
						normalRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						normalRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t modifiedRegenTime = (1000.f * (Entity::getHungerTickRate(stats[player.playernum], true, false)) / static_cast<float>(TICKS_PER_SECOND));
						modifiedRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						modifiedRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t baseValue = 100.0 * (normalRegenTime / modifiedRegenTime);
						real_t displayedValue = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);

						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_bonus_format").c_str(), displayedValue - baseValue);
					}
					else
					{
						Monster type = stats[player.playernum]->type;
						bool aestheticOnly = false;
						if ( player.entity )
						{
							if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
							{
								if ( stats[player.playernum]->appearance != 0 )
								{
									aestheticOnly = true;
									type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
								}
							}
						}
						real_t baseRegen = 100.0;
						if ( type == SKELETON )
						{
							baseRegen = 25.0;
						}

						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_items_bonus").c_str());
						Sint32 oldINT = stats[player.playernum]->INT;
						stats[player.playernum]->INT += -statGetINT(stats[player.playernum], player.entity);
						real_t regenWithoutINT = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						stats[player.playernum]->INT = oldINT;
						real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						real_t regenStatSkill = regenTotal - regenWithoutINT;

						real_t regenItemsEffects = regenTotal - regenStatSkill - baseRegen;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regenItemsEffects);
					}
				}
					break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					//real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					//real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);

					real_t displayValue = (currentEquippedSpeedPercent - noWeightSpeedPercent);
					if ( displayValue >= 0.0 )
					{
						displayValue = -.000001; // so there is a negative sign
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_equipment_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						displayValue);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[4] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
			++currentTextBackingFrameIndex;
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 5, buf, valueBuf)
			|| (element != SHEET_ATK && element != SHEET_RES && !(element == SHEET_RGN && !(svFlags & SV_FLAG_HUNGER))) )
		{
			// extra number display - line 5
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_passive_bonus").c_str());

					Sint32 oldSkillLVL = stats[player.playernum]->PROFICIENCIES[PRO_SHIELD];
					bool oldDefending = stats[player.playernum]->defending;
					stats[player.playernum]->defending = false;

					Sint32 armor = AC(stats[player.playernum]);
					stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] = 0;
					Sint32 armorNoSkill = AC(stats[player.playernum]);
					stats[player.playernum]->PROFICIENCIES[PRO_SHIELD] = oldSkillLVL;
					stats[player.playernum]->defending = oldDefending;

					Sint32 passiveBonus = armor - armorNoSkill;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_bonus_format").c_str(), passiveBonus);
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_entry_items_bonus").c_str());
					// maybe one day add intrinsic spell power buffs. for now, 0
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_bonus_format").c_str(), 0);
				}
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
					real_t regen = (static_cast<real_t>(Entity::getHealthRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
					if ( regen <= 0.0 )
					{
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_hp_per_second_format_zero").c_str(),
							(static_cast<real_t>(HEAL_TIME) / TICKS_PER_SECOND));
					}
					else
					{
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_hp_per_second_format").c_str(),
							regen);
					}
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_ht_boiler_status").c_str());
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							value = -1;
							snprintf(valueBuf, sizeof(valueBuf), "%s", getHoverTextString("attributes_rgn_ht_boiler_value_low").c_str());
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							value = 2;
							snprintf(valueBuf, sizeof(valueBuf), "%s", getHoverTextString("attributes_rgn_ht_boiler_value_superheat").c_str());
						}
						else
						{
							value = 1;
							snprintf(valueBuf, sizeof(valueBuf), "%s", getHoverTextString("attributes_rgn_ht_boiler_value_normal").c_str());
						}
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
						real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_per_second_format").c_str(), regen);
					}
					else
					{
						Monster type = stats[player.playernum]->type;
						bool aestheticOnly = false;
						if ( player.entity )
						{
							if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
							{
								if ( stats[player.playernum]->appearance != 0 )
								{
									aestheticOnly = true;
									type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
								}
							}
						}
						real_t baseRegen = 100.0;
						if ( type == SKELETON )
						{
							baseRegen = 25.0;
						}

						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_statskill_bonus").c_str());
						Sint32 oldINT = stats[player.playernum]->INT;
						stats[player.playernum]->INT += -statGetINT(stats[player.playernum], player.entity);
						real_t regenWithoutINT = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						stats[player.playernum]->INT = oldINT;

						real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						real_t regenStatSkill = regenTotal - regenWithoutINT;

						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regenStatSkill);
					}
				}
					break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					int goldWeightTotal = stats[player.playernum]->GOLD / 100;
					int goldWeight = player.movement.getCharacterModifiedWeight(&goldWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					real_t goldSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(goldWeight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);
					real_t goldSpeedPercent = 100.0 * goldSpeed / std::fmax(.01, maxSpeed);

					real_t displayValue = (currentSpeedPercent - noWeightSpeedPercent)
						- (currentEquippedSpeedPercent - noWeightSpeedPercent)
						- (goldSpeedPercent - noWeightSpeedPercent);
					if ( displayValue >= 0.0 )
					{
						displayValue = -.000001; // so there is a negative sign
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_inventory_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						displayValue);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				if ( isAutomatonHTRegen )
				{
					if ( value == 2 ) // supercharge
					{
						entryValue->setColor(hudColors.characterSheetHeadingText);
					}
					else
					{
						entryValue->setColor(hudColors.characterSheetGreen);
					}
				}
				else
				{
					entryValue->setColor(hudColors.characterSheetGreen);
				}
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			if ( element == SHEET_RGN || (element == SHEET_RGN_MP && (isAutomatonHTRegen || isInsectoidENRegen)) )
			{
				// special rule here to ignore size of this long line
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				//longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				//valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(true);

				backingFramePos.w = (int)txtValueGet->getWidth();
				backingFramePos.x -= backingFramePos.w;
				backingFramePos.x -= 8;
				entryValue->setSize(backingFramePos);
				++currentTextBackingFrameIndex;
			}
			else
			{
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(false);
				++currentTextBackingFrameIndex;
			}
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 6, buf, valueBuf)
			|| (element == SHEET_RGN_MP && !isInsectoidENRegen) || element == SHEET_WGT )
		{
			// extra number display - line 6
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
						real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							regen /= 6; // degrade faster
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_per_second_format").c_str(), -1, regen);
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
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_per_second_format").c_str(), 1, regen);
						}
						else if ( stats[player.playernum]->HUNGER > 300 )
						{
							// normal manaRegenInterval 300-1200 hunger.
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_per_second_format").c_str(), 1, regen);
						}
					}
					else
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
						real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_mp_per_second_format").c_str(), regen);
					}
				}
					break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					//int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					//int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					int goldWeightTotal = stats[player.playernum]->GOLD / 100;
					int goldWeight = player.movement.getCharacterModifiedWeight(&goldWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					//real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					real_t goldSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(goldWeight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					//real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);
					real_t goldSpeedPercent = 100.0 * goldSpeed / std::fmax(.01, maxSpeed);

					real_t displayValue = (goldSpeedPercent - noWeightSpeedPercent);
					if ( displayValue >= 0.0 )
					{
						displayValue = -.000001; // so there is a negative sign
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_gold_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						displayValue);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			if ( element == SHEET_RGN_MP )
			{
				// special rule here to ignore size of this long line
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				//longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				//valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(true);

				backingFramePos.w = (int)txtValueGet->getWidth();
				backingFramePos.x -= backingFramePos.w;
				backingFramePos.x -= 8;
				entryValue->setSize(backingFramePos);
				++currentTextBackingFrameIndex;
			}
			else
			{
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(false);
				++currentTextBackingFrameIndex;
			}
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 7, buf, valueBuf) )
		{
			// extra number display - line 7
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
			++currentTextBackingFrameIndex;
		}

		for ( int index = 1; index <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++index )
		{
			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][index];
			if ( txtValueBackingFrame->isDisabled() )
			{
				continue;
			}

			if ( valueSizes.find(index) == valueSizes.end() )
			{
				continue;
			}

			SDL_Rect valuePos = valueSizes[index].second;
			Field* entryValue = valueSizes[index].first;
			SDL_Rect entryValuePos = entryValue->getSize();
			entryValuePos.x = entryValuePos.x + entryValuePos.w;
			entryValuePos.w = (int)longestValue;
			entryValuePos.x -= entryValuePos.w;
			entryValuePos.x -= 8;
			entryValue->setSize(entryValuePos);

			valuePos.w = (int)longestValue + 16;
			valuePos.x -= (valuePos.w);
			valuePos.y -= 3;
			valuePos.h += 4;

			txtValueBackingFrame->setSize(valuePos);

			imageResizeToContainer9x9(txtValueBackingFrame, SDL_Rect{ 0, 0, valuePos.w, valuePos.h }, skillsheetEffectBackgroundImages);
		}

		if ( !hasEntryInfoLines )
		{
			currentHeight += padyMid * 2;
		}

		{
			currentHeight += padyMid;

			div->pos.x = padx;
			div->pos.y = currentHeight;
			div->pos.w = txtPos.w;
			div->disabled = false;

			currentHeight += padyMid;

			auto entry = characterSheetTooltipTextFields[player.playernum][15]; assert(entry);
			entry->setDisabled(false);
			char buf[512] = "";

			std::string descTextFormatted = "\x1E ";
			for ( auto s : descText )
			{
				descTextFormatted += s;
				if ( s == '\n' )
				{
					descTextFormatted += "\x1E ";
				}
			}

			snprintf(buf, sizeof(buf), "%s", descTextFormatted.c_str());
			entry->setText(buf);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w;
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			if ( element == SHEET_RGN && !(svFlags & SV_FLAG_HUNGER) )
			{
				entry->setColor(hudColors.itemContextMenuHeadingText);
			}
			else
			{
				entry->setColor(hudColors.characterSheetOffWhiteText);
			}
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);

			currentHeight += padyMid / 4;
			tooltipPos.h = pady1 + currentHeight + pady2;
		}

		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;
		tooltipFrame->setSize(tooltipPos);
		if ( tooltipPos.y + tooltipPos.h > sheetFrame->getSize().h )
		{
			// keep on-screen
			tooltipPos.y -= ((tooltipPos.y + tooltipPos.h) - sheetFrame->getSize().h);
			tooltipFrame->setSize(tooltipPos);
		}
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			skillsheetEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR )
	{
		const int maxWidth = 260;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		
		std::string descriptionText = mapDisplayNamesDescriptions[map.name].second.c_str();
		std::string mapDetailsText = "";
		auto mapDetails = Player::Minimap_t::mapDetails;
		for ( auto& detail : mapDetails )
		{
			if ( mapDetailsText != "" )
			{
				mapDetailsText += '\n';
			}
			mapDetailsText += "\x1E ";
			mapDetailsText += detail.second;
		}

		txt->setText(descriptionText.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txt->setSize(txtPos);

		tooltipPos.w = txtGet->getWidth() + padx * 2;

		int currentHeight = txtPos.y + txtPos.h - 4;

		if ( Player::Minimap_t::mapDetails.size() > 0 )
		{
			currentHeight += padyMid;
			auto entry = tooltipFrame->findField("txt 1"); assert(entry);
			entry->setDisabled(false);
			entry->setText(mapDetailsText.c_str());

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (2 * (padx + padxMid));
			entry->setSize(entryPos);
			entry->reflowTextToFit(0);
			entryPos.h = actualFont->height(true) * entry->getNumTextLines();
			entry->setSize(entryPos);
			entry->setColor(makeColor(255, 0, 255, 255));
			currentHeight = std::max(entryPos.y + entryPos.h, 0);
		}

		if ( Player::Minimap_t::mapDetails.size() > 0 )
		{
			tooltipPos.h = pady1 + currentHeight + pady2;
		}
		else
		{
			tooltipPos.h = pady1 + txtPos.h + pady2;
		}
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
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
		bool usingMouse = !inputs.getVirtualMouse(player.playernum)->lastMovementFromController;
		txt->setText(getHoverTextString("gold_mouse").c_str());

		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + padyMid;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txt->setSize(txtPos);
		
		tooltipPos.w = txtPos.w + padx * 2;
		tooltipPos.h = pady1 + txtPos.h + pady2;

		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			skillsheetEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_TIMER )
	{
		const int maxWidth = 240;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 8;
		const int padyMid = 4;
		const int padxMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		bool usingMouse = !inputs.getVirtualMouse(player.playernum)->lastMovementFromController;

		if ( player.characterSheet.showGameTimerAlways )
		{
			std::string tooltiptxt = getHoverTextString("game_timer_mouse") + getHoverTextString("game_timer_unpin");
			txt->setText(tooltiptxt.c_str());
		}
		else
		{
			std::string tooltiptxt = getHoverTextString("game_timer_mouse") + getHoverTextString("game_timer_pin");
			txt->setText(tooltiptxt.c_str());
		}

		int currentHeight = padyMid;

		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		txt->reflowTextToFit(0);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + padyMid;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txt->setSize(txtPos);
		
		currentHeight = std::max(currentHeight, txtPos.h);

		tooltipPos.w = txtPos.w + padx * 2;
		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
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

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick;
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& inputs.hasController(player.playernum)
		&& !Input::inputs[player.playernum].binary("MenuConfirm") )
	{
		enableTooltips = false;
	}

	bool bCompactView = player.bUseCompactGUIHeight();

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
			int offsetx = 0;
			if ( stats[player.playernum]->sex == sex_t::MALE )
			{
				if ( type == AUTOMATON )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_AutomatonM_02.png";
				}
				else
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_M_02.png";
					static ConsoleVariable<int> cvar_sexoffset("/sexoffsetx", -1);
					offsetx = *cvar_sexoffset;
				}
			}
			else if ( stats[player.playernum]->sex == sex_t::FEMALE )
			{
				if ( type == AUTOMATON )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_AutomatonF_02.png";
				}
				else
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_F_02.png";
				}
			}
			if ( auto imgGet = Image::get(sexImg->path.c_str()) )
			{
				sexImg->pos.w = (int)imgGet->getWidth();
				sexImg->pos.h = (int)imgGet->getHeight();
			}

			SDL_Rect raceTextPos = raceText->getSize();
			raceTextPos.x = 4;
			if ( centerIconAndText )
			{
				raceTextPos.x = 4 + (((sexImg->pos.w + 4)) / 2) + offsetx;
			}
			if ( raceTextPos.x % 2 == 1 )
			{
				--raceTextPos.x;
			}
			raceText->setSize(raceTextPos);

			sexImg->pos.x = 16;
			if ( centerIconAndText )
			{
				sexImg->pos.x = raceText->getSize().x + raceText->getSize().w / 2 - width / 2;
				sexImg->pos.x -= (sexImg->pos.w + 4) + (offsetx * 2);
			}
			if ( sexImg->pos.x % 2 == 1 )
			{
				--sexImg->pos.x;
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
				if ( selectedElement == SHEET_DUNGEON_FLOOR && enableTooltips )
				{
					SDL_Rect tooltipPos = characterInfoFrame->getSize();
					tooltipPos.y = floorFrame->getSize().y;
					Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
					if ( panelJustify == PANEL_JUSTIFY_LEFT || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
					{
						tooltipJustify = PANEL_JUSTIFY_LEFT;
						tooltipPos.x += tooltipPos.w;
					}
					updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
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
			if ( Input::inputs[player.playernum].binary("MenuRightClick")
				&& player.bControlEnabled && !gamePaused
				&& !player.usingCommand()
				&& player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
				&& !player.GUI.isDropdownActive() )
			{
				player.GUI.dropdownMenu.open("drop_gold");
			}
			else if ( (!inputs.getVirtualMouse(player.playernum)->draw_cursor
					&& inputs.hasController(player.playernum)
					&& Input::inputs[player.playernum].binaryToggle("MenuConfirm")
				)
				&& player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
				&& !player.GUI.isDropdownActive()
				&& !player.usingCommand()
				&& player.bControlEnabled && !gamePaused )
			{
				Input::inputs[player.playernum].consumeBinaryToggle("MenuConfirm");
				player.GUI.dropdownMenu.open("drop_gold");
				player.GUI.dropdownMenu.dropDownToggleClick = true;
				SDL_Rect dropdownPos = characterInfoFrame->getSize();
				dropdownPos.y += gold->getSize().y + gold->getSize().h / 2;
				if ( auto interactMenuTop = player.GUI.dropdownMenu.dropdownFrame->findImage("interact top background") )
				{
					// 10px is slot half height, move by 1.5 slots, minus the top interact text height
					dropdownPos.y -= (interactMenuTop->pos.h + (3 * 10) + 4);
				}
				if ( !player.GUI.dropdownMenu.getDropDownAlignRight("drop_gold") )
				{
					player.GUI.dropdownMenu.dropDownX = dropdownPos.x;
				}
				else
				{
					player.GUI.dropdownMenu.dropDownX = dropdownPos.x + dropdownPos.w;
				}
				player.GUI.dropdownMenu.dropDownX += player.camera_virtualx1();
				player.GUI.dropdownMenu.dropDownY = dropdownPos.y + player.camera_virtualy1();
			}
			if ( enableTooltips && !inputs.getVirtualMouse(player.playernum)->lastMovementFromController )
			{
				SDL_Rect tooltipPos = characterInfoFrame->getSize();
				tooltipPos.y += gold->getSize().y;
				Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
				if ( panelJustify == PANEL_JUSTIFY_LEFT || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
				{
					tooltipJustify = PANEL_JUSTIFY_LEFT;
					tooltipPos.x += tooltipPos.w;
				}
				updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
			}
		}
	}
}

void Player::CharacterSheet_t::updateStats()
{
	auto characterInfoFrame = sheetFrame->findFrame("character info");
	assert(characterInfoFrame);
	auto characterInnerFrame = characterInfoFrame->findFrame("character info inner frame");
	assert(characterInnerFrame);
	auto statsFrame = sheetFrame->findFrame("stats");
	assert(statsFrame);

	auto statsPos = statsFrame->getSize();
	//statsPos.x = sheetFrame->getSize().w - statsPos.w;
	statsFrame->setSize(statsPos);

	auto statsInnerFrame = statsFrame->findFrame("stats inner frame");
	assert(statsInnerFrame);

	const int rightAlignPosX = 0;
	const int leftAlignPosX = 10;
	auto statsInnerPos = statsInnerFrame->getSize();
	statsInnerPos.x = rightAlignPosX;
	if ( keystatus[SDL_SCANCODE_I] && enableDebugKeys )
	{
		statsInnerPos.x = leftAlignPosX;
	}
	statsInnerFrame->setSize(statsInnerPos);

	Button* strButton = statsInnerFrame->findButton("str button");
	Button* dexButton = statsInnerFrame->findButton("dex button");
	Button* conButton = statsInnerFrame->findButton("con button");
	Button* intButton = statsInnerFrame->findButton("int button");
	Button* perButton = statsInnerFrame->findButton("per button");
	Button* chrButton = statsInnerFrame->findButton("chr button");

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick;
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor 
		&& inputs.hasController(player.playernum)
		&& !Input::inputs[player.playernum].binary("MenuConfirm") )
	{
		enableTooltips = false;
	}

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
		if ( selectedElement == SHEET_STR && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
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
		if ( selectedElement == SHEET_DEX && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
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
		if ( selectedElement == SHEET_CON && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
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
		if ( selectedElement == SHEET_INT && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
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
		if ( selectedElement == SHEET_PER && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
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
		if ( selectedElement == SHEET_CHR && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}
}

void Player::CharacterSheet_t::updateAttributes()
{
	auto attributesFrame = sheetFrame->findFrame("attributes");
	assert(attributesFrame);

	auto attributesPos = attributesFrame->getSize();
	//attributesPos.x = sheetFrame->getSize().w - attributesPos.w;
	attributesFrame->setSize(attributesPos);

	auto attributesInnerFrame = attributesFrame->findFrame("attributes inner frame");
	assert(attributesInnerFrame);

	const int rightAlignPosX = 0;
	const int leftAlignPosX = 10;
	auto attributesInnerPos = attributesInnerFrame->getSize();
	attributesInnerPos.x = rightAlignPosX;
	/*if ( keystatus[SDL_SCANCODE_I] )
	{
		attributesInnerPos.x = leftAlignPosX;
	}*/
	attributesInnerFrame->setSize(attributesInnerPos);

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick;
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& inputs.hasController(player.playernum)
		&& !Input::inputs[player.playernum].binary("MenuConfirm") )
	{
		enableTooltips = false;
	}

	char buf[32] = "";

	if ( auto field = attributesInnerFrame->findField("atk text stat") )
	{
		AttackHoverText_t atkHoverText;
		Sint32 displayedATK = displayAttackPower(player.playernum, atkHoverText);
		if ( atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF
			|| atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL
			|| atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP
			|| atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_DEFAULT )
		{
			snprintf(buf, sizeof(buf), "-");
		}
		else
		{
			snprintf(buf, sizeof(buf), "%d", displayedATK);
		}
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_ATK && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}

	if ( auto field = attributesInnerFrame->findField("ac text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", AC(stats[player.playernum]));
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_AC && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}

	if ( auto field = attributesInnerFrame->findField("pwr text stat") )
	{
		real_t spellPower = (getBonusFromCasterOfSpellElement(player.entity, stats[player.playernum]) * 100.0) + 100.0;
		snprintf(buf, sizeof(buf), "%.f%%", spellPower);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_POW && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}

	if ( auto field = attributesInnerFrame->findField("res text stat") )
	{
		real_t resistance = 100.0 * hit.entity->getDamageTableMultiplier(*stats[player.playernum], DAMAGE_TABLE_MAGIC);
		resistance /= (Entity::getMagicResistance(stats[player.playernum]) + 1);
		resistance = -(resistance - 100.0);
		snprintf(buf, sizeof(buf), "%d%%", (int)resistance);
		field->setText(buf);
		field->setColor(hudColors.characterSheetNeutral);
		if ( resistance > 0.01 )
		{
			field->setColor(hudColors.characterSheetGreen);
		}
		else if ( resistance < -0.01 )
		{
			field->setColor(hudColors.characterSheetRed);
		}

		if ( selectedElement == SHEET_RES && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}

	if ( auto field = attributesInnerFrame->findField("regen text hp") )
	{
		field->setColor(hudColors.characterSheetNeutral);
		Uint32 color = hudColors.characterSheetNeutral;
		getDisplayedHPRegen(players[player.playernum]->entity, *stats[player.playernum], &color, buf);
		field->setText(buf);
		field->setColor(color);

		if ( selectedElement == SHEET_RGN && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}

	if ( auto field = attributesInnerFrame->findField("regen text mp") )
	{
		field->setColor(hudColors.characterSheetNeutral);
		Uint32 color = hudColors.characterSheetNeutral;
		getDisplayedMPRegen(players[player.playernum]->entity, *stats[player.playernum], &color, buf);
		field->setText(buf);
		field->setColor(color);

		if ( selectedElement == SHEET_RGN_MP && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
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

		if ( selectedElement == SHEET_WGT && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			updateCharacterSheetTooltip(selectedElement, tooltipPos);
		}
	}
}

void Player::Hotbar_t::processHotbar()
{
	if ( !hotbarFrame )
	{
		char name[32];
		snprintf(name, sizeof(name), "player hotbar %d", player.playernum);
		hotbarFrame = player.hud.hudFrame->addFrame(name);
		hotbarFrame->setHollow(true);
		hotbarFrame->setBorder(0);
		hotbarFrame->setOwner(player.playernum);
		createHotbar(player.playernum);
	}
	hotbarFrame->setSize(SDL_Rect{ 0, 0,
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	if ( gamePaused || nohud || !players[player.playernum]->isLocalPlayer() )
	{
		// hide
		hotbarFrame->setDisabled(true);
		return;
	}
	else
	{
		hotbarFrame->setDisabled(false);
	}

	updateHotbar();
}

void Player::Inventory_t::Appraisal_t::updateAppraisalAnim()
{
	/*if ( current_item == 0 )
	{
		animAppraisal = 0.0;
		return;
	}*/
	real_t fpsScale = (60.f / std::max(1U, fpsLimit));
	real_t scale = PI / 40;
	animAppraisal += fpsScale * (scale);
	if ( animAppraisal >= 4 * PI )
	{
		animAppraisal -= 4 * PI;
	}

	if ( itemNotifyUpdatedThisTick == 0 )
	{
		itemNotifyUpdatedThisTick = ticks;
		itemNotifyAnimState = 0;
	}
	if ( (itemNotifyUpdatedThisTick != ticks) && (ticks - itemNotifyUpdatedThisTick) % (TICKS_PER_SECOND / 5) == 0 )
	{
		if ( itemNotifyUpdatedThisTick != ticks )
		{
			++itemNotifyAnimState;
			itemNotifyUpdatedThisTick = ticks;
		}
		if ( itemNotifyAnimState > 2 )
		{
			itemNotifyAnimState = 0;
		}
	}
}

void drawUnidentifiedItemEffectHotbarCallback(const Widget& widget, SDL_Rect rect)
{
	const int player = widget.getOwner();
	auto& appraisal = players[player]->inventoryUI.appraisal;
	if ( appraisal.animStartTick == ticks )
	{
		return;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glViewport(0, 0, Frame::virtualScreenX, Frame::virtualScreenY);
	glOrtho(0, Frame::virtualScreenX, 0, Frame::virtualScreenY, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// build a circle mesh
	static std::vector<std::pair<real_t, real_t>> circle_mesh;
	if ( !circle_mesh.size() ) {
		circle_mesh.emplace_back((real_t)0.0, (real_t)0.0);
		static const int num_circle_vertices = 32;
		for ( int c = 0; c <= num_circle_vertices; ++c ) {
			real_t ang = (PI / 2) - ((PI * 2.0) / num_circle_vertices) * c;
			circle_mesh.emplace_back((real_t)(cos(ang) / 2.0), -(real_t)(sin(ang) / 2.0));
		}
	}
	static std::vector<std::pair<real_t, real_t>> square_mesh;
	if ( !square_mesh.size() ) {
		square_mesh.emplace_back((real_t)0.0, (real_t)0.0);
		static const int num_square_vertices = 200;
		int increment = 25;
		real_t x = 0.0;
		real_t y = -0.5;
		int dir = 0;
		int iterations = 0;
		for ( int c = 0; c <= num_square_vertices; ++c ) {
			square_mesh.emplace_back(x, y);

			if ( dir == 0 )
			{
				y = -0.5;
				x += 1.01 / increment;
				if ( x >= 0.5 )
				{
					dir = 1;
				}
				x = std::min(x, 0.5);
			}
			else if ( dir == 1 )
			{
				x = 0.5;
				y += 1.01 / increment;
				if ( y >= 0.5 )
				{
					dir = 2;
				}
				y = std::min(y, 0.5);
			}
			else if ( dir == 2 )
			{
				y = 0.5;
				x -= 1.01 / increment;
				if ( x <= -0.5 )
				{
					dir = 3;
				}
				x = std::max(x, -0.5);
			}
			else if ( dir == 3 )
			{
				x = -0.5;
				y -= 1.01 / increment;
				if ( y <= -0.5 )
				{
					dir = 4;
				}
				y = std::max(y, -0.5);
			}
			else if ( dir == 4 )
			{
				y = -0.5;
				x += 1.01 / increment;
				if ( x >= 0.0 )
				{
					square_mesh.emplace_back(0.0, y);
					++iterations;
					break;
				}
				x = std::min(x, 0.0);
			}
		}
	}

	const Frame* parent = static_cast<const Frame*>(widget.getParent());

	auto drawSquareMesh = [](const int player, real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		const real_t unitX = (real_t)rect.w;
		const real_t unitY = (real_t)rect.h;
		Uint8 r, g, b, a;

		// bind a texture to circle mesh
		auto testImage = Image::get("images/ui/HUD/hotbar/Appraisal_Icon_OutlineHotbar.png");
		testImage->bind();

		getColor(color, &r, &g, &b, &a);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
		glFrontFace(GL_CW); // we draw clockwise so need to set this.
		glBegin(GL_TRIANGLE_FAN);
		auto& appraisal = players[player]->inventoryUI.appraisal;
		int numVertices = (((double)(appraisal.timermax - appraisal.timer)) / ((double)appraisal.timermax)) * square_mesh.size() + 1;
		int index = 0;
		for ( auto pair = square_mesh.begin(); pair != square_mesh.end(); ++pair ) {
			const real_t sx = (*pair).first * unitX * size;
			const real_t sy = (*pair).second * unitY * size;

			float const tx = ((*pair).first / 1.0) + 0.5;
			float const ty = ((*pair).second / 1.0) + 0.5;
			glTexCoord2f(tx, ty);
			glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
			if ( index >= numVertices )
			{
				break;
			}
			++index;
		}
		glEnd();
		glFrontFace(GL_CCW);
	};
	{
		SDL_Rect drawRect = rect;
		drawRect.x += 4;
		drawRect.y += 4;
		drawRect.w -= 6;
		drawRect.h -= 6;
		real_t opacity = 192;
		if ( parent && parent->getOpacity() < 100.0 )
		{
			opacity *= parent->getOpacity() / 100.0;
		}
		drawSquareMesh(player, drawRect.x + drawRect.w / 2, drawRect.y + drawRect.h / 2, 1.0, drawRect, makeColor(255, 255, 255, opacity));
	}

	auto drawMesh = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		Uint8 r, g, b, a;

		// bind a texture to mesh
		auto testImage = Image::get("images/ui/Inventory/Appraisal_Icon.png");
		testImage->bind();

		const real_t sx = rect.w * size;
		const real_t sy = rect.h * size;

		getColor(color, &r, &g, &b, &a);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(x, Frame::virtualScreenY - y);
		glTexCoord2f(0, 1);
		glVertex2f(x, Frame::virtualScreenY - (y + sy));
		glTexCoord2f(1, 1);
		glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
		glTexCoord2f(1, 0);
		glVertex2f(x + sx, Frame::virtualScreenY - y);
		glEnd();
	};
	{
		const int imgSize = 26;
		SDL_Rect drawRect = rect;
		drawRect.x += 4 + (rect.w - 6) / 2 - imgSize / 2;
		drawRect.y += 4 + (rect.h - 6) / 2 - imgSize / 2;
		int offsetSize = 7;
		int offsetx = offsetSize * cos(std::min(4 * PI, appraisal.animAppraisal));
		int offsety = offsetSize * sin(std::min(4 * PI, appraisal.animAppraisal));

		drawRect.w = imgSize;
		drawRect.h = imgSize;

		real_t opacity = 92 + 160 * fabs(sin(std::min(2 * PI, appraisal.animAppraisal) / 2));
		if ( parent && parent->getOpacity() < 100.0 )
		{
			opacity *= parent->getOpacity() / 100.0;
		}
		drawMesh(drawRect.x + offsetx, drawRect.y + offsety,
			(real_t)1.0, drawRect,
			makeColor(255, 255, 255, opacity));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void drawUnidentifiedItemEffectCallback(const Widget& widget, SDL_Rect rect)
{
	const int player = widget.getOwner();
	auto& appraisal = players[player]->inventoryUI.appraisal;
	if ( appraisal.animStartTick == ticks )
	{
		return;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glViewport(0, 0, Frame::virtualScreenX, Frame::virtualScreenY);
	glOrtho(0, Frame::virtualScreenX, 0, Frame::virtualScreenY, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// build a circle mesh
	static std::vector<std::pair<real_t, real_t>> circle_mesh;
	if ( !circle_mesh.size() ) {
		circle_mesh.emplace_back((real_t)0.0, (real_t)0.0);
		static const int num_circle_vertices = 32;
		for ( int c = 0; c <= num_circle_vertices; ++c ) {
			real_t ang = (PI / 2) - ((PI * 2.0) / num_circle_vertices) * c;
			circle_mesh.emplace_back((real_t)(cos(ang) / 2.0), -(real_t)(sin(ang) / 2.0));
		}
	}
	static std::vector<std::pair<real_t, real_t>> square_mesh;
	if ( !square_mesh.size() ) {
		square_mesh.emplace_back((real_t)0.0, (real_t)0.0);
		static const int num_square_vertices = 200;
		int increment = 25;
		real_t x = 0.0;
		real_t y = -0.5;
		int dir = 0;
		int iterations = 0;
		for ( int c = 0; c <= num_square_vertices; ++c ) {
			square_mesh.emplace_back(x, y);
			
			if ( dir == 0 )
			{
				y = -0.5;
				x += 1.01 / increment;
				if ( x >= 0.5 )
				{
					dir = 1;
				}
				x = std::min(x, 0.5);
			}
			else if ( dir == 1 )
			{
				x = 0.5;
				y += 1.01 / increment;
				if ( y >= 0.5 )
				{
					dir = 2;
				}
				y = std::min(y, 0.5);
			}
			else if ( dir == 2 )
			{
				y = 0.5;
				x -= 1.01 / increment;
				if ( x <= -0.5 )
				{
					dir = 3;
				}
				x = std::max(x, -0.5);
			}
			else if ( dir == 3 )
			{
				x = -0.5;
				y -= 1.01 / increment;
				if ( y <= -0.5 )
				{
					dir = 4;
				}
				y = std::max(y, -0.5);
			}
			else if ( dir == 4 )
			{
				y = -0.5;
				x += 1.01 / increment;
				if ( x >= 0.0 )
				{
					square_mesh.emplace_back(0.0, y);
					++iterations;
					break;
				}
				x = std::min(x, 0.0);
			}
		}
	}

	const Frame* parent = static_cast<const Frame*>(widget.getParent());

	auto drawSquareMesh = [](const int player, real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		const real_t unitX = (real_t)rect.w;
		const real_t unitY = (real_t)rect.h;
		Uint8 r, g, b, a;

		// bind a texture to circle mesh
		auto testImage = Image::get("images/ui/Inventory/Appraisal_Icon_Outline.png");
		testImage->bind();

		getColor(color, &r, &g, &b, &a);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
		glFrontFace(GL_CW); // we draw clockwise so need to set this.
		glBegin(GL_TRIANGLE_FAN);
		auto& appraisal = players[player]->inventoryUI.appraisal;
		int numVertices = (((double)(appraisal.timermax - appraisal.timer)) / ((double)appraisal.timermax)) * square_mesh.size() + 1;
		int index = 0;
		for ( auto pair = square_mesh.begin(); pair != square_mesh.end(); ++pair ) {
			const real_t sx = (*pair).first * unitX * size;
			const real_t sy = (*pair).second * unitY * size;

			float const tx = ((*pair).first / 1.0) + 0.5;
			float const ty = ((*pair).second / 1.0) + 0.5;
			glTexCoord2f(tx, ty);
			glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
			if ( index >= numVertices )
			{
				break;
			}
			++index;
		}
		glEnd();
		glFrontFace(GL_CCW);
	};
	{
		SDL_Rect drawRect = rect;
		drawRect.x += 2;
		drawRect.y += 2;
		drawRect.w -= 2;
		drawRect.h -= 2;
		real_t opacity = 192;
		if ( parent && parent->getOpacity() < 100.0 )
		{
			opacity *= parent->getOpacity() / 100.0;
		}
		drawSquareMesh(player, drawRect.x + drawRect.w / 2, drawRect.y + drawRect.h / 2, 1.0, drawRect, makeColor(255, 255, 255, opacity));
	}
	auto drawMesh = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		Uint8 r, g, b, a;

		// bind a texture to mesh
		auto testImage = Image::get("images/ui/Inventory/Appraisal_Icon.png");
		testImage->bind();

		const real_t sx = rect.w * size;
		const real_t sy = rect.h * size;

		getColor(color, &r, &g, &b, &a);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(x, Frame::virtualScreenY - y);
		glTexCoord2f(0, 1);
		glVertex2f(x, Frame::virtualScreenY - (y + sy));
		glTexCoord2f(1, 1);
		glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
		glTexCoord2f(1, 0);
		glVertex2f(x + sx, Frame::virtualScreenY - y);
		glEnd();
	};
	{
		const int imgSize = 26;
		SDL_Rect drawRect = rect;
		drawRect.x += 4 + (rect.w - 6) / 2 - imgSize / 2;
		drawRect.y += 4 + (rect.h - 6) / 2 - imgSize / 2;
		int offsetSize = 7;
		int offsetx = offsetSize * cos(std::min(4 * PI, appraisal.animAppraisal));
		int offsety = offsetSize * sin(std::min(4 * PI, appraisal.animAppraisal));

		drawRect.w = imgSize;
		drawRect.h = imgSize;

		real_t opacity = 92 + 160 * fabs(sin(std::min(2 * PI, appraisal.animAppraisal) / 2));
		if ( parent && parent->getOpacity() < 100.0 )
		{
			opacity *= parent->getOpacity() / 100.0;
		}
		drawMesh(drawRect.x + offsetx, drawRect.y + offsety, 
			(real_t)1.0, drawRect, 
			makeColor(255, 255, 255, opacity));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
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
	brokenStatusFrame->addImage(coloredBackgroundPos, makeColor( 160, 160, 160, 64), "images/system/white.png", "broken status bg");

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
	auto img = itemSpriteFrame->addImage(imgPos, 0xFFFFFFFF, "images/system/white.png", "item sprite img");
	img->outline = false;
	img->outlineColor = 0;
	auto iconLabelBgImg = itemSpriteFrame->addImage(SDL_Rect{ 0, 0, 16, 16 }, 0xFFFFFFFF,
		"images/ui/Inventory/Icon_Label_Backing_00.png", "icon label bg img");
	iconLabelBgImg->disabled = true;

	auto iconLabelImg = itemSpriteFrame->addImage(SDL_Rect{ 0, 0, 16, 16 }, 0xFFFFFFFF,
		"", "icon label img");
	iconLabelImg->disabled = true;

	auto unusableFrame = slotFrame->addFrame("unusable item frame");
	unusableFrame->setSize(slotSize);
	unusableFrame->setHollow(true);
	unusableFrame->setDisabled(true);
	unusableFrame->addImage(coloredBackgroundPos, makeColor( 64, 64, 64, 144), "images/system/white.png", "unusable item bg");

	auto appraisalFrame = slotFrame->addFrame("appraisal frame");
	appraisalFrame->setSize(slotSize);
	appraisalFrame->setHollow(true);
	appraisalFrame->setDisabled(true);
	appraisalFrame->addImage(SDL_Rect{ 4, 4, 6, 14 }, 0xFFFFFFFF,
		"images/ui/Inventory/tooltips/ExclamationAnim00.png", "new notif img");

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
	SDL_Rect equippedImgPos = { 2, slotSize.h - 18, 18, 18 };
	equippedIconFrame->addImage(equippedImgPos, 0xFFFFFFFF, "images/system/Equipped.png", "equipped icon img");

	auto brokenIconFrame = slotFrame->addFrame("broken icon frame");
	brokenIconFrame->setSize(slotSize);
	brokenIconFrame->setHollow(true);
	brokenIconFrame->addImage(equippedImgPos, 0xFFFFFFFF, "images/system/Broken.png", "broken icon img");
}

void resetInventorySlotFrames(const int player)
{
	for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
	{
		for ( int y = Player::Inventory_t::PaperDollRows::DOLL_ROW_1; y < players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY + players[player]->inventoryUI.getPlayerBackpackBonusSizeY(); ++y )
		{
			if ( auto slotFrame = players[player]->inventoryUI.getInventorySlotFrame(x, y) )
			{
				slotFrame->setDisabled(true);
			}
		}
	}

	for ( int x = 0; x < Player::Inventory_t::MAX_SPELLS_X; ++x )
	{
		for ( int y = 0; y < Player::Inventory_t::MAX_SPELLS_Y; ++y )
		{
			if ( auto slotFrame = players[player]->inventoryUI.getSpellSlotFrame(x, y) )
			{
				slotFrame->setDisabled(true);
			}
		}
	}

	for ( int x = 0; x < Player::Inventory_t::MAX_CHEST_X; ++x )
	{
		for ( int y = 0; y < Player::Inventory_t::MAX_CHEST_Y; ++y )
		{
			if ( auto slotFrame = players[player]->inventoryUI.getChestSlotFrame(x, y) )
			{
				slotFrame->setDisabled(true);
			}
		}
	}

	for ( int x = 0; x < Player::ShopGUI_t::MAX_SHOP_X; ++x )
	{
		for ( int y = 0; y < Player::ShopGUI_t::MAX_SHOP_Y; ++y )
		{
			if ( auto slotFrame = players[player]->shopGUI.getShopSlotFrame(x, y) )
			{
				slotFrame->setDisabled(true);
			}
		}
	}
}

bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy, bool spells)
{
	if ( !gui )
	{
		return false;
	}

	if ( players[player]->inventoryUI.chestFrame && !spells
		&& !players[player]->inventoryUI.chestFrame->isDisabled() )
	{
		for ( int x = 0; x < Player::Inventory_t::MAX_CHEST_X; ++x )
		{
			for ( int y = 0; y < Player::Inventory_t::MAX_CHEST_Y; ++y )
			{
				auto slotFrame = players[player]->inventoryUI.getChestSlotFrame(x, y);
				if ( !slotFrame )
				{
					continue;
				}

				if ( !players[player]->inventoryUI.chestGUI.isSlotVisible(x, y) )
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
	if ( players[player]->inventoryUI.frame && !spells 
		&& players[player]->inventory_mode == INVENTORY_MODE_ITEM )
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
	if ( players[player]->inventoryUI.spellFrame && spells 
		&& players[player]->inventory_mode == INVENTORY_MODE_SPELL )
	{
		for ( int x = 0; x < Player::Inventory_t::MAX_SPELLS_X; ++x )
		{
			for ( int y = 0; y < Player::Inventory_t::MAX_SPELLS_Y; ++y )
			{
				auto slotFrame = players[player]->inventoryUI.getSpellSlotFrame(x, y);
				if ( !slotFrame )
				{
					continue;
				}

				if ( !players[player]->inventoryUI.spellPanel.isSlotVisible(x, y) )
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

void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr, bool forceUnusable)
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
	if ( !strcmp(slotFrame->getName(), "dragging inventory item") ) // dragging item, no need for colors
	{
		disableBackgrounds = true;
	}

	int* slotType = nullptr;
	if ( slotFrame->getUserData() )
	{
		slotType = (int*)slotFrame->getUserData();
		if ( *slotType == GAMEUI_FRAMEDATA_ANIMATING_ITEM || *slotType == GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT )
		{
			disableBackgrounds = true;
		}
	}

	bool isHotbarIcon = false;
	bool alchemyResultIcon = &GenericGUI[player].alchemyGUI.alchemyResultPotion == item;
	if ( spriteImage->path != "" )
	{
		spriteImageFrame->setDisabled(false);
		if ( inputs.getUIInteraction(player)->selectedItem == item )
		{
			if ( !strcmp(slotFrame->getName(), "hotbar slot item") ) // hotbar slots
			{
				// fade this icon
				spriteImage->color = makeColor( 255, 255, 255, 128);
				disableBackgrounds = true;
				isHotbarIcon = true;
			}
		}
		else if ( !strcmp(slotFrame->getName(), "hotbar slot item") ) // hotbar slots
		{
			isHotbarIcon = true;
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
						spriteImage->color = makeColor( 255, 255, 255, 128);
					}
				}
			}
		}
		else
		{
			spriteImage->color = 0xFFFFFFFF;
		}
		if ( auto iconLabelImg = spriteImageFrame->findImage("icon label img") )
		{
			iconLabelImg->path = ItemTooltips.getIconLabel(*item);
			iconLabelImg->disabled = true;
			const int size = 16;
			const int padx = spriteImageFrame->getSize().w / 2 - size / 2;
			iconLabelImg->pos = SDL_Rect{ spriteImageFrame->getSize().w - size - 1,
				1 /*spriteImageFrame->getSize().h - size*/, size, size };
			if ( iconLabelImg->path != "" )
			{
				iconLabelImg->disabled = !item->identified;
			}
			iconLabelImg->color = spriteImage->color;
			if ( auto iconLabelBgImg = spriteImageFrame->findImage("icon label bg img") )
			{
				iconLabelBgImg->pos.w = 24;
				iconLabelBgImg->pos.h = iconLabelBgImg->pos.w;
				iconLabelBgImg->pos.x = spriteImageFrame->getSize().w - iconLabelBgImg->pos.w - 1;
				iconLabelBgImg->pos.y = 1;
				iconLabelBgImg->disabled = iconLabelImg->disabled || disableBackgrounds;
				iconLabelBgImg->color = makeColor(255, 255, 255, 255);
			}
		}
		if ( slotFrame->getUserData() )
		{
			if ( *slotType == GAMEUI_FRAMEDATA_ALCHEMY_ITEM || *slotType == GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT )
			{
				SDL_Color color;
				getColor(spriteImage->color, &color.r, &color.g, &color.b, &color.a);
				color.a /= 2;
				spriteImage->color = makeColor(color.r, color.g, color.b, color.a);
			}
		}
	}

	if ( auto qtyFrame = slotFrame->findFrame("quantity frame") )
	{
		qtyFrame->setDisabled(true);
		bool drawQty = (item->count > 1) ? true : false;
		if ( !drawQty && GenericGUI[player].isNodeTinkeringCraftableItem(item->node) )
		{
			drawQty = true;
		}
		else if ( slotType && *slotType == GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_ENTRY )
		{
			drawQty = true;
		}
		Uint32 qtyColor = 0xFFFFFFFF;
		bool stackable = false;
		Item*& selectedItem = inputs.getUIInteraction(player)->selectedItem;
		if ( selectedItem && !isHotbarIcon && !alchemyResultIcon 
			&& !(slotType 
				&& (*slotType == GAMEUI_FRAMEDATA_ANIMATING_ITEM 
					|| *slotType == GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT
					|| *slotType == GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_ENTRY)) )
		{
			if ( item != selectedItem 
				&& !itemIsEquipped(selectedItem, player)
				&& !itemIsEquipped(item, player) )
			{
				int selectedItemQty;
				int destItemQty;
				auto result = getItemStackingBehavior(player, selectedItem, item, selectedItemQty, destItemQty);
				if ( result.resultType == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK
					|| result.resultType == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
				{
					drawQty = true;
					qtyColor = hudColors.characterSheetGreen;
					stackable = true;
				}
			}
		}
		if ( drawQty )
		{
			qtyFrame->setDisabled(false);
			if ( auto qtyText = qtyFrame->findField("quantity text") )
			{
				char qtybuf[32] = "";
				if ( stackable )
				{
					if ( item->count == 1 )
					{
						snprintf(qtybuf, sizeof(qtybuf), "+");
					}
					else if ( item->count > 1 )
					{
						snprintf(qtybuf, sizeof(qtybuf), "%d+", item->count);
					}
				}
				else
				{
					snprintf(qtybuf, sizeof(qtybuf), "%d", item->count);
				}
				qtyText->setText(qtybuf);
				qtyText->setColor(qtyColor);
			}
		}
	}
	
	if ( auto beatitudeFrame = slotFrame->findFrame("beatitude status frame") )
	{
		beatitudeFrame->setDisabled(true);
		spriteImage->outline = false;
		if ( !disableBackgrounds )
		{
			if ( auto beatitudeImg = beatitudeFrame->findImage("beatitude status bg") )
			{
				if ( !item->identified )
				{
					beatitudeImg->color = makeColor( 128, 128, 0, 125);
					beatitudeFrame->setDisabled(false);
					//spriteImage->outlineColor = makeColor(210, 183, 76, 255);
					//spriteImage->outline = true;
				}
				else if ( item->beatitude < 0 )
				{
					beatitudeImg->color = makeColor( 128, 0, 0, 125);
					beatitudeFrame->setDisabled(false);
					//spriteImage->outlineColor = hudColors.characterSheetRed;
					//spriteImage->outline = true;
				}
				else if ( item->beatitude > 0 )
				{
					if ( colorblind )
					{
						beatitudeImg->color = makeColor( 100, 245, 255, 65);
					}
					else
					{
						beatitudeImg->color = makeColor( 0, 255, 0, 65);
					}
					//spriteImage->outlineColor = hudColors.characterSheetHeadingText;
					//spriteImage->outline = true;
					beatitudeFrame->setDisabled(false);
				}
				if ( !spriteImage->outline )
				{
					spriteImage->outlineColor = 0;
				}
				else
				{
					spriteImage->outlineColor = makeColor(0, 0, 0, 255);
				}
				if ( !beatitudeFrame->isDisabled() )
				{
					if ( isHotbarIcon )
					{
						beatitudeImg->color = makeColor(255, 255, 255, 255);
						if ( !item->identified )
						{
							beatitudeImg->path = "*#images/ui/HUD/hotbar/HUD_Quickbar_Slot_Box_Overlay_App01.png";
						}
						else if ( item->beatitude > 0 )
						{
							beatitudeImg->path = "*#images/ui/HUD/hotbar/HUD_Quickbar_Slot_Box_Overlay_Bless01.png";
						}
						else if ( item->beatitude < 0 )
						{
							beatitudeImg->path = "*#images/ui/HUD/hotbar/HUD_Quickbar_Slot_Box_Overlay_Curse01.png";
						}
					}
					else
					{
						beatitudeImg->color = makeColor(255, 255, 255, 255);
						if ( !item->identified )
						{
							beatitudeImg->path = "*#images/ui/Inventory/HUD_Inventory_Item_App00B.png";
						}
						else if ( item->beatitude > 0 )
						{
							beatitudeImg->path = "*#images/ui/Inventory/HUD_Inventory_Item_Bless00B.png";
						}
						else if ( item->beatitude < 0 )
						{
							beatitudeImg->path = "*#images/ui/Inventory/HUD_Inventory_Item_Curse00B.png";
						}
						//beatitudeImg->path = "images/system/white.png";
					}
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
				auto brokenStatusImg = brokenStatusFrame->findImage("broken status bg");
				if ( isHotbarIcon )
				{
					brokenStatusImg->path = "*#images/ui/HUD/hotbar/HUD_Quickbar_Slot_Box_Overlay_01.png";
				}
				else
				{
					brokenStatusImg->path = "images/system/white.png";
				}
			}
		}
	}

	if ( auto unusableFrame = slotFrame->findFrame("unusable item frame") )
	{
		bool greyedOut = forceUnusable;
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
		else if ( alchemyResultIcon && stats[player]->weapon 
			&& itemCategory(stats[player]->weapon) == POTION
			&& stats[player]->weapon->identified && item->identified )
		{
			int selectedItemQty;
			int destItemQty;
			auto result = getItemStackingBehavior(player, item, stats[player]->weapon, selectedItemQty, destItemQty);
			if ( result.resultType == ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK
				|| result.resultType == ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK )
			{
				equipped = true;
			}
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
		if ( equipped && (!disableBackgrounds || (slotType && (*slotType == GAMEUI_FRAMEDATA_ANIMATING_ITEM))) )
		{
			equippedIconFrame->setDisabled(false);
		}
	}
	if ( auto brokenIconFrame = slotFrame->findFrame("broken icon frame") )
	{
		brokenIconFrame->setDisabled(true);
		if ( broken && (!disableBackgrounds || (slotType && (*slotType == GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT))) )
		{
			brokenIconFrame->setDisabled(false);
		}
	}

	if ( auto appraisalFrame = slotFrame->findFrame("appraisal frame") )
	{
		appraisalFrame->setDisabled(true);
		appraisalFrame->setDrawCallback(nullptr);
		if ( item->notifyIcon )
		{
			appraisalFrame->setDisabled(false);
		}
		if ( !disableBackgrounds && players[player]->inventoryUI.appraisal.current_item > 0
			&& slotFrame->getOpacity() > 0.999
			&& !item->identified && item->node && item->node->list == &stats[player]->inventory
			&& item->uid == players[player]->inventoryUI.appraisal.current_item )
		{
			if ( isHotbarIcon )
			{
				appraisalFrame->setDisabled(false);
				appraisalFrame->setDrawCallback([](const Widget& widget, SDL_Rect rect) {
					drawUnidentifiedItemEffectHotbarCallback(widget, rect);
				});
			}
			else
			{
				appraisalFrame->setDisabled(false);
				appraisalFrame->setDrawCallback([](const Widget& widget, SDL_Rect rect) {
					drawUnidentifiedItemEffectCallback(widget, rect);
				});
			}
		}
		if ( !appraisalFrame->isDisabled() )
		{
			auto img = appraisalFrame->findImage("new notif img");
			img->disabled = true;
			if ( item->notifyIcon )
			{
				img->pos.x = 4;
				img->pos.y = 4;
				if ( isHotbarIcon && !players[player]->hotbar.useHotbarFaceMenu )
				{
					img->pos.x = 10;
					img->pos.y = 2;
					if ( players[player]->hotbar.slots()[NUM_HOTBAR_SLOTS - 1].item == item->uid )
					{
						if ( slotFrame && slotFrame->getParent() && !strcmp(slotFrame->getParent()->getName(), "hotbar slot 9") )
						{
							img->pos.x += 6;
						}
					}
				}
				img->disabled = false;
				auto& animState = players[player]->inventoryUI.appraisal.itemNotifyAnimState;
				switch ( animState )
				{
					case 0:
						img->path = "images/ui/Inventory/tooltips/ExclamationAnim00.png";
						break;
					case 1:
						img->path = "images/ui/Inventory/tooltips/ExclamationAnim01.png";
						break;
					case 2:
						img->path = "images/ui/Inventory/tooltips/ExclamationAnim02.png";
						break;
					default:
						img->path = "images/ui/Inventory/tooltips/ExclamationAnim00.png";
						break;
				}
			}
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
		players[player]->inventoryUI.tooltipFrame = gameUIFrame[player]->addFrame(name);
		auto tooltipFrame = players[player]->inventoryUI.tooltipFrame;
		tooltipFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipFrame->setHollow(true);
		tooltipFrame->setDisabled(true);
		tooltipFrame->setInheritParentFrameOpacity(false);
	}
	else
	{
		return;
	}

	auto tooltipFrame = players[player]->inventoryUI.tooltipFrame;

	Uint32 color = makeColor( 255, 255, 255, 255);
	tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 28 },
		color, "*#images/ui/Inventory/tooltips/Hover_T00.png", "tooltip top background");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 28 },
		color, "*#images/ui/Inventory/tooltips/Hover_TL00.png", "tooltip top left");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 28 },
		color, "*#images/ui/Inventory/tooltips/Hover_TR00.png", "tooltip top right");

	tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 52 },
		color, "*#images/ui/Inventory/tooltips/Hover_C00.png", "tooltip middle background");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 52 },
		color, "*#images/ui/Inventory/tooltips/Hover_L00.png", "tooltip middle left");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 52 },
		color, "*#images/ui/Inventory/tooltips/Hover_R00.png", "tooltip middle right");

	tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 26 },
		color, "*#images/ui/Inventory/tooltips/Hover_B00.png", "tooltip bottom background");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 26 },
		color, "*#images/ui/Inventory/tooltips/Hover_BL01.png", "tooltip bottom left");
	tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 26 },
		color, "*#images/ui/Inventory/tooltips/Hover_BR01.png", "tooltip bottom right");

	//const std::string headerFont = "fonts/pixelmix.ttf#14#2";
	//const std::string bodyFont = "fonts/pixelmix.ttf#12#2";
	const std::string headerFont = "fonts/pixel_maz_multiline.ttf#16#2";
	const std::string bodyFont = "fonts/pixel_maz_multiline.ttf#16#2";

	auto tooltipTextField = tooltipFrame->addField("inventory mouse tooltip header", 1024);
	tooltipTextField->setText("Nothing");
	tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
	tooltipTextField->setFont(headerFont.c_str());
	tooltipTextField->setHJustify(Field::justify_t::LEFT);
	tooltipTextField->setVJustify(Field::justify_t::CENTER);
	tooltipTextField->setColor(makeColor( 67, 195, 157, 255));

	// temporary debug stuff
	{
		Frame::image_t* tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
		0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip min");
		tmp->color = makeColor( 255, 0, 0, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip max");
		tmp->color = makeColor( 0, 255, 0, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip header max");
		tmp->color = makeColor( 0, 255, 255, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip header bg");
		tmp->color = makeColor( 255, 255, 255, 255);
		tmp->disabled = true;
		tmp = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip header bg new");
		tmp->color = makeColor( 255, 255, 0, 255);
		tmp->disabled = true;
	}

	if ( auto attrFrame = tooltipFrame->addFrame("inventory mouse tooltip attributes frame") )
	{
		attrFrame->setHollow(true);
		attrFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		auto spellImageBg = attrFrame->addImage(SDL_Rect{ 0, 0, 52, 52 },
			0xFFFFFFFF, "*#images/ui/Inventory/tooltips/SpellBorder_00.png", "inventory mouse tooltip spell image bg");
		spellImageBg->disabled = true;
		//spellImageBg->color = makeColor( 125, 125, 125, 228);
		auto spellImage = attrFrame->addImage(SDL_Rect{ 0, 0, 40, 40 },
			0xFFFFFFFF, "images/system/white.png", "inventory mouse tooltip spell image");
		spellImage->disabled = true;

		attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			0xFFFFFFFF, "*#images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Damage_00.png", "inventory mouse tooltip primary image");
		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value highlight", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value positive text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value negative text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value slot name", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::RIGHT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(0xFFFFFFFF);

		attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			0xFFFFFFFF, "images/system/con32.png", "inventory mouse tooltip secondary image");
		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value highlight", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value positive text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value negative text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			0xFFFFFFFF, "images/system/con32.png", "inventory mouse tooltip third image");
		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value highlight", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value positive text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip third value negative text", 256);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = attrFrame->addField("inventory mouse tooltip attributes text", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		tooltipTextField->setColor(makeColor( 188, 154, 114, 255));
	}
	if ( auto descFrame = tooltipFrame->addFrame("inventory mouse tooltip description frame") )
	{
		descFrame->setHollow(true);
		descFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		descFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
			makeColor( 49, 53, 61, 255),
			"images/system/white.png", "inventory mouse tooltip description divider");

		tooltipTextField = descFrame->addField("inventory mouse tooltip description", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		//tooltipTextField->setColor(makeColor( 188, 154, 114, 255));
		//tooltipTextField->setColor(0xFFFFFFFF);
		tooltipTextField->setTextColor(makeColor( 67, 195, 157, 255));

		tooltipTextField = descFrame->addField("inventory mouse tooltip description positive text", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		//tooltipTextField->setColor(makeColor( 1, 151, 246, 255));
		tooltipTextField->setColor(0xFFFFFFFF);
		tooltipTextField->setTextColor(makeColor( 188, 154, 114, 255));

		tooltipTextField = descFrame->addField("inventory mouse tooltip description negative text", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		tooltipTextField->setColor(0xFFFFFFFF);
		tooltipTextField->setTextColor(makeColor( 215, 38, 61, 255));
	}
	if ( auto valueFrame = tooltipFrame->addFrame("inventory mouse tooltip value frame") )
	{
		valueFrame->setHollow(true);
		valueFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		valueFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			makeColor( 49, 53, 61, 255), 
			"images/system/white.png", "inventory mouse tooltip value background");

		valueFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
			makeColor( 49, 53, 61, 255),
			"images/system/white.png", "inventory mouse tooltip value divider");

		tooltipTextField = valueFrame->addField("inventory mouse tooltip identified value", 64);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(makeColor( 188, 154, 114, 255));

		valueFrame->addImage(SDL_Rect{ 0, 0, 16, 16 },
			0xFFFFFFFF, 
			"*#images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Money_00.png",
			"inventory mouse tooltip gold image");

		tooltipTextField = valueFrame->addField("inventory mouse tooltip gold value", 64);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(makeColor( 188, 154, 114, 255));

		valueFrame->addImage(SDL_Rect{ 0, 0, 16, 16 },
			0xFFFFFFFF, 
			"*#images/ui/Inventory/tooltips/HUD_Tooltip_Icon_WGT_00.png",
			"inventory mouse tooltip weight image");

		tooltipTextField = valueFrame->addField("inventory mouse tooltip weight value", 64);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::LEFT);
		tooltipTextField->setVJustify(Field::justify_t::CENTER);
		tooltipTextField->setColor(makeColor( 188, 154, 114, 255));
	}
	if ( auto promptFrame = tooltipFrame->addFrame("inventory mouse tooltip prompt frame") )
	{
		promptFrame->setHollow(true);
		promptFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

		tooltipTextField = promptFrame->addField("inventory mouse tooltip prompt", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont(bodyFont.c_str());
		tooltipTextField->setHJustify(Field::justify_t::RIGHT);
		tooltipTextField->setVJustify(Field::justify_t::TOP);
		tooltipTextField->setColor(makeColor( 148, 82, 3, 255));

	}
	auto tooltipPromptImg = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "", "inventory mouse tooltip prompt img");
	tooltipPromptImg->disabled = true;

	snprintf(name, sizeof(name), "player interact %d", player);
	if ( auto interactFrame = gameUIFrame[player]->addFrame(name) )
	{
		players[player]->inventoryUI.interactFrame = interactFrame;
		const int interactWidth = 106;
		interactFrame->setSize(SDL_Rect{ 0, 0, interactWidth + 6 * 2, 100 });
		interactFrame->setDisabled(true);
		interactFrame->setInheritParentFrameOpacity(false);

		Uint32 color = makeColor( 255, 255, 255, 255);
		const int topBackgroundHeight = 30;
		const int optionHeight = 20;

		interactFrame->addImage(SDL_Rect{ 24, 0, 0, 30 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_T03.png", "interact top background");
		interactFrame->addImage(SDL_Rect{ 0, 0, 24, 30 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_TL03.png", "interact top left");
		interactFrame->addImage(SDL_Rect{ 0, 0, 24, 30 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_TR03.png", "interact top right");

		interactFrame->addImage(SDL_Rect{ 24, 30, 0, 12 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_C03.png", "interact middle background");
		auto ml = interactFrame->addImage(SDL_Rect{ 0, 30, 24, 12 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_L03.png", "interact middle left");
		ml->tiled = true;
		auto mr = interactFrame->addImage(SDL_Rect{ 0, 30, 24, 12 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_R03.png", "interact middle right");
		mr->tiled = true;

		interactFrame->addImage(SDL_Rect{ 24, 96, 0, 14 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_B03.png", "interact bottom background");
		interactFrame->addImage(SDL_Rect{ 0, 96, 24, 14 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_BL03.png", "interact bottom left");
		interactFrame->addImage(SDL_Rect{ 0, 96, 24, 14 },
			color, "*#images/ui/Inventory/tooltips/HoverItemMenu_BR03.png", "interact bottom right");

		auto selectmid = interactFrame->addImage(SDL_Rect{ 6, optionHeight - 16, interactWidth, 22 },
			hudColors.itemContextMenuOptionSelectedImg, "*#images/ui/Inventory/tooltips/HoverItemMenu_SelectBack_M03.png", "interact selected highlight mid");
		selectmid->tiled = true;
		interactFrame->addImage(SDL_Rect{ 6, optionHeight - 16, 20, 22 },
			hudColors.itemContextMenuOptionSelectedImg, "*#images/ui/Inventory/tooltips/HoverItemMenu_SelectBack_L03.png", "interact selected highlight left");
		interactFrame->addImage(SDL_Rect{ 6, optionHeight - 16, 20, 22 },
			hudColors.itemContextMenuOptionSelectedImg, "*#images/ui/Inventory/tooltips/HoverItemMenu_SelectBack_R03.png", "interact selected highlight right");


		const char* interactFont = "fonts/pixel_maz.ttf#32#2";

		auto interactText = interactFrame->addField("interact text", 32);
		interactText->setText(language[4040]);
		interactText->setSize(SDL_Rect{ 0, 2, 0, topBackgroundHeight });
		interactText->setFont(interactFont);
		interactText->setHJustify(Field::justify_t::CENTER);
		interactText->setVJustify(Field::justify_t::CENTER);
		interactText->setColor(hudColors.itemContextMenuHeadingText);

		const int interactOptionStartX = 4;
		const int interactOptionStartY = 33;
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

		textAlignY = interactGlyph2->pos.y - 10;
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

		textAlignY = interactGlyph3->pos.y - 10;
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

		textAlignY = interactGlyph4->pos.y - 10;
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

		textAlignY = interactGlyph5->pos.y - 10;
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
	if ( auto promptFrame = gameUIFrame[player]->addFrame(name) )
	{
		players[player]->inventoryUI.tooltipPromptFrame = promptFrame;
		const int interactWidth = 0;
		SDL_Rect promptSize{ 0, 0, interactWidth + 6 * 2, 100 };
		promptFrame->setDisabled(true);
		promptFrame->setInheritParentFrameOpacity(false);

		Uint32 color = makeColor( 255, 255, 255, 192);

		auto middleCenter = promptFrame->addImage(SDL_Rect{ 6, 2, interactWidth, 76 },
			color, "*#images/ui/Inventory/tooltips/Hover_C00.png", "interact middle background");
		auto middleTop = promptFrame->addImage(SDL_Rect{ 6, 0, interactWidth, 2 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_C00.png", "interact middle top background");
		auto middleBottom = promptFrame->addImage(SDL_Rect{ 6, 0, interactWidth, 2 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_C00.png", "interact middle bottom background");
		auto middleLeft = promptFrame->addImage(SDL_Rect{ 0, 0, 6, 76 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_L00.png", "interact middle left");
		auto middleRight = promptFrame->addImage(SDL_Rect{ interactWidth + 6, 0, 6, 76 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_R00.png", "interact middle right");

		auto bottomCenter = promptFrame->addImage(SDL_Rect{ 4, 76, interactWidth + 4, 4 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_B00.png", "interact bottom background");
		auto bottomLeft = promptFrame->addImage(SDL_Rect{ 0, 76, 4, 4 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_BL00.png", "interact bottom left");
		auto bottomRight = promptFrame->addImage(SDL_Rect{ interactWidth + 4 * 2, 76, 4, 4 },
			color, "*#images/ui/Inventory/tooltips/HoverExt_BR00.png", "interact bottom right");

		const int interactOptionStartX = 60;
		const int interactOptionStartY = 4;
		const int glyphSizeH = 24;
		const int glyphSizeW = 22;

		auto interactGlyph1 = promptFrame->addImage(
			SDL_Rect{ interactOptionStartX, interactOptionStartY, glyphSizeW, glyphSizeH },
			0xFFFFFFFF, "", "glyph 1");

		auto interactGlyph2 = promptFrame->addImage(
			SDL_Rect{ interactGlyph1->pos.x - (glyphSizeW / 2),
			interactGlyph1->pos.y + glyphSizeH,
			glyphSizeW, glyphSizeH },
			0xFFFFFFFF, "", "glyph 2");

		auto interactGlyph3 = promptFrame->addImage(
			SDL_Rect{ interactGlyph1->pos.x + (glyphSizeW + glyphSizeW / 4), interactGlyph1->pos.y, glyphSizeW, glyphSizeH },
			0xFFFFFFFF, "", "glyph 3");

		auto interactGlyph4 = promptFrame->addImage(
			SDL_Rect{ interactGlyph3->pos.x - (glyphSizeW / 2), interactGlyph2->pos.y,
			glyphSizeW, glyphSizeH },
			0xFFFFFFFF, "", "glyph 4");

		const int textWidth = 200;
		const int textHeight = glyphSizeH + 8;

		Uint32 promptTextColor = makeColor( 188, 154, 114, 255);
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

		bottomCenter->pos.y = interactGlyph4->pos.y + interactGlyph4->pos.h + 2;
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

void drawCharacterPreview(const int player, SDL_Rect pos, int fov, real_t offsetyaw)
{
	view_t view;
	auto ofov = ::fov;
	::fov = fov;

	//TempTexture* minimapTexture = new TempTexture();

	if ( players[player] != nullptr && players[player]->entity != nullptr )
	{
		glClear(GL_DEPTH_BUFFER_BIT);

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
						if ( (*itr).HasMember("icon_base_path_32px") )
						{
							entry.skillIconPath32px = (*itr)["icon_base_path_32px"].GetString();
						}
						if ( (*itr).HasMember("icon_legend_path_32px") )
						{
							entry.skillIconPathLegend32px = (*itr)["icon_legend_path_32px"].GetString();
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
						skillSheetData.defaultTextColor = makeColor(
							d["colors"]["default"]["r"].GetInt(),
							d["colors"]["default"]["g"].GetInt(),
							d["colors"]["default"]["b"].GetInt(),
							d["colors"]["default"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("novice") )
					{
						skillSheetData.noviceTextColor = makeColor(
							d["colors"]["novice"]["r"].GetInt(),
							d["colors"]["novice"]["g"].GetInt(),
							d["colors"]["novice"]["b"].GetInt(),
							d["colors"]["novice"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("expert") )
					{
						skillSheetData.expertTextColor = makeColor(
							d["colors"]["expert"]["r"].GetInt(),
							d["colors"]["expert"]["g"].GetInt(),
							d["colors"]["expert"]["b"].GetInt(),
							d["colors"]["expert"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("legend") )
					{
						skillSheetData.legendTextColor = makeColor(
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
				if ( d.HasMember("selected_cursor_opacity") )
				{
					selectedCursorOpacity = d["selected_cursor_opacity"].GetInt();
				}
				if ( d.HasMember("selected_old_cursor_opacity") )
				{
					oldSelectedCursorOpacity = d["selected_old_cursor_opacity"].GetInt();
				}
				if ( d.HasMember("hotbar") )
				{
					if ( d["hotbar"].HasMember("hotbar_slot_opacity") )
					{
						hotbarSlotOpacity = d["hotbar"]["hotbar_slot_opacity"].GetInt();
					}
					if ( d["hotbar"].HasMember("hotbar_selected_slot_opacity") )
					{
						hotbarSelectedSlotOpacity = d["hotbar"]["hotbar_selected_slot_opacity"].GetInt();
					}
					if ( d["hotbar"].HasMember("hotbar_compact_x_offset") )
					{
						hotbarCompactOffsetX = d["hotbar"]["hotbar_compact_x_offset"].GetInt();
					}
					if ( d["hotbar"].HasMember("hotbar_compact_slot_overlap") )
					{
						hotbarCompactSlotOverlapPercent = d["hotbar"]["hotbar_compact_slot_overlap"].GetDouble();
					}
					if ( d["hotbar"].HasMember("hotbar_compact_inactive_slot_movement_x") )
					{
						hotbarCompactInactiveSlotMovementX = d["hotbar"]["hotbar_compact_inactive_slot_movement_x"].GetInt();
					}
					if ( d["hotbar"].HasMember("hotbar_compact_expanded_x_offset") )
					{
						hotbarCompactExpandedOffsetX = d["hotbar"]["hotbar_compact_expanded_x_offset"].GetInt();
					}
					if ( d["hotbar"].HasMember("hotbar_y_offset") )
					{
						hotbarOffsetY = d["hotbar"]["hotbar_y_offset"].GetInt();
					}
					if ( d["hotbar"].HasMember("hotbar_compact_y_offset") )
					{
						hotbarCompactOffsetY = d["hotbar"]["hotbar_compact_y_offset"].GetInt();
					}
				}
				if ( d.HasMember("xpbar") )
				{
					if ( d["xpbar"].HasMember("xpbar_width_offset") )
					{
						xpbarOffsetWidth = d["xpbar"]["xpbar_width_offset"].GetInt();
					}
					if ( d["xpbar"].HasMember("xpbar_compact_width_offset") )
					{
						xpbarCompactOffsetWidth = d["xpbar"]["xpbar_compact_width_offset"].GetInt();
					}
					if ( d["xpbar"].HasMember("xpbar_y_offset") )
					{
						xpbarOffsetY = d["xpbar"]["xpbar_y_offset"].GetInt();
					}
					if ( d["xpbar"].HasMember("xpbar_compact_y_offset") )
					{
						xpbarCompactOffsetY = d["xpbar"]["xpbar_compact_y_offset"].GetInt();
					}
				}
				if ( d.HasMember("hpmpbar") )
				{
					if ( d["hpmpbar"].HasMember("hpmpbar_width_offset") )
					{
						hpmpbarOffsetWidth = d["hpmpbar"]["hpmpbar_width_offset"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_x_offset") )
					{
						hpmpbarOffsetX = d["hpmpbar"]["hpmpbar_x_offset"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_y_offset") )
					{
						hpmpbarOffsetY = d["hpmpbar"]["hpmpbar_y_offset"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_width_offset") )
					{
						hpmpbarCompactOffsetWidth = d["hpmpbar"]["hpmpbar_compact_width_offset"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_x_offset") )
					{
						hpmpbarCompactOffsetX = d["hpmpbar"]["hpmpbar_compact_x_offset"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_y_offset") )
					{
						hpmpbarCompactOffsetY = d["hpmpbar"]["hpmpbar_compact_y_offset"].GetInt();
					}

					if ( d["hpmpbar"].HasMember("hpmpbar_max_amount_threshold") )
					{
						hpmpbarMaxWidthAmount = d["hpmpbar"]["hpmpbar_max_amount_threshold"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_interval_to_increase_width") )
					{
						hpmpbarIntervalToIncreaseWidth 
							= d["hpmpbar"]["hpmpbar_interval_to_increase_width"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_base_percent") )
					{
						hpmpbarBasePercentSize = d["hpmpbar"]["hpmpbar_base_percent"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_interval_start_value") )
					{
						hpmpbarIntervalStartValue = d["hpmpbar"]["hpmpbar_interval_start_value"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_width_increase_percent_on_interval") )
					{
						hpmpbarWidthIncreasePercentOnInterval = 
							d["hpmpbar"]["hpmpbar_width_increase_percent_on_interval"].GetDouble();
					}

					if ( d["hpmpbar"].HasMember("hpmpbar_compact_max_amount_threshold") )
					{
						hpmpbarCompactMaxWidthAmount = d["hpmpbar"]["hpmpbar_compact_max_amount_threshold"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_interval_to_increase_width") )
					{
						hpmpbarCompactIntervalToIncreaseWidth = 
							d["hpmpbar"]["hpmpbar_compact_interval_to_increase_width"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_base_percent") )
					{
						hpmpbarCompactBasePercentSize = d["hpmpbar"]["hpmpbar_compact_base_percent"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_interval_start_value") )
					{
						hpmpbarCompactIntervalStartValue = d["hpmpbar"]["hpmpbar_compact_interval_start_value"].GetInt();
					}
					if ( d["hpmpbar"].HasMember("hpmpbar_compact_width_increase_percent_on_interval") )
					{
						hpmpbarCompactWidthIncreasePercentOnInterval = 
							d["hpmpbar"]["hpmpbar_compact_width_increase_percent_on_interval"].GetDouble();
					}
				}
				if ( d.HasMember("action_prompts") )
				{
					if ( d["action_prompts"].HasMember("x_offset") )
					{
						Player::HUD_t::actionPromptOffsetX = d["action_prompts"]["x_offset"].GetInt();
					}
					if ( d["action_prompts"].HasMember("y_offset") )
					{
						Player::HUD_t::actionPromptOffsetY = d["action_prompts"]["y_offset"].GetInt();
					}
					if ( d["action_prompts"].HasMember("icon_size") )
					{
						Player::HUD_t::actionPromptIconSize = d["action_prompts"]["icon_size"].GetInt();
					}
					if ( d["action_prompts"].HasMember("icon_backing_size") )
					{
						Player::HUD_t::actionPromptBackingSize = d["action_prompts"]["icon_backing_size"].GetInt();
					}
					if ( d["action_prompts"].HasMember("icon_opacity") )
					{
						Player::HUD_t::actionPromptIconOpacity = d["action_prompts"]["icon_opacity"].GetInt();
					}
					if ( d["action_prompts"].HasMember("icon_backing_opacity") )
					{
						Player::HUD_t::actionPromptIconBackingOpacity = d["action_prompts"]["icon_backing_opacity"].GetInt();
					}
					if ( d["action_prompts"].HasMember("prompt_img_00") )
					{
						actionPromptBackingIconPath00 = d["action_prompts"]["prompt_img_00"].GetString();
					}
					if ( d["action_prompts"].HasMember("prompt_img_20") )
					{
						actionPromptBackingIconPath20 = d["action_prompts"]["prompt_img_20"].GetString();
					}
					if ( d["action_prompts"].HasMember("prompt_img_60") )
					{
						actionPromptBackingIconPath60 = d["action_prompts"]["prompt_img_60"].GetString();
					}
					if ( d["action_prompts"].HasMember("prompt_img_100") )
					{
						actionPromptBackingIconPath100 = d["action_prompts"]["prompt_img_100"].GetString();
					}
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
					if ( d["enemy_hp_bars"].HasMember("quick_fade_delay_ticks") )
					{
						EnemyHPDamageBarHandler::shortDistanceHPBarFadeTicks = d["enemy_hp_bars"]["quick_fade_delay_ticks"].GetInt();
					}
					if ( d["enemy_hp_bars"].HasMember("quick_fade_distance_from_player_multiplier") )
					{
						if ( d["enemy_hp_bars"]["quick_fade_distance_from_player_multiplier"].IsDouble() )
						{
							EnemyHPDamageBarHandler::shortDistanceHPBarFadeDistance = d["enemy_hp_bars"]["quick_fade_distance_from_player_multiplier"].GetDouble();
						}
						else
						{
							EnemyHPDamageBarHandler::shortDistanceHPBarFadeDistance = d["enemy_hp_bars"]["quick_fade_distance_from_player_multiplier"].GetInt();
						}
					}
				}
				if ( d.HasMember("colors") )
				{
					if ( d["colors"].HasMember("itemmenu_heading_text") )
					{
						hudColors.itemContextMenuHeadingText = makeColor(
							d["colors"]["itemmenu_heading_text"]["r"].GetInt(),
							d["colors"]["itemmenu_heading_text"]["g"].GetInt(),
							d["colors"]["itemmenu_heading_text"]["b"].GetInt(),
							d["colors"]["itemmenu_heading_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_option_text") )
					{
						hudColors.itemContextMenuOptionText = makeColor(
							d["colors"]["itemmenu_option_text"]["r"].GetInt(),
							d["colors"]["itemmenu_option_text"]["g"].GetInt(),
							d["colors"]["itemmenu_option_text"]["b"].GetInt(),
							d["colors"]["itemmenu_option_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_selected_text") )
					{
						hudColors.itemContextMenuOptionSelectedText = makeColor(
							d["colors"]["itemmenu_selected_text"]["r"].GetInt(),
							d["colors"]["itemmenu_selected_text"]["g"].GetInt(),
							d["colors"]["itemmenu_selected_text"]["b"].GetInt(),
							d["colors"]["itemmenu_selected_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_selected_img") )
					{
						hudColors.itemContextMenuOptionSelectedImg = makeColor(
							d["colors"]["itemmenu_selected_img"]["r"].GetInt(),
							d["colors"]["itemmenu_selected_img"]["g"].GetInt(),
							d["colors"]["itemmenu_selected_img"]["b"].GetInt(),
							d["colors"]["itemmenu_selected_img"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("itemmenu_option_img") )
					{
						hudColors.itemContextMenuOptionImg = makeColor(
							d["colors"]["itemmenu_option_img"]["r"].GetInt(),
							d["colors"]["itemmenu_option_img"]["g"].GetInt(),
							d["colors"]["itemmenu_option_img"]["b"].GetInt(),
							d["colors"]["itemmenu_option_img"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_neutral_text") )
					{
						hudColors.characterSheetNeutral = makeColor(
							d["colors"]["charsheet_neutral_text"]["r"].GetInt(),
							d["colors"]["charsheet_neutral_text"]["g"].GetInt(),
							d["colors"]["charsheet_neutral_text"]["b"].GetInt(),
							d["colors"]["charsheet_neutral_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_neutral_light_text") )
					{
						hudColors.characterSheetLightNeutral = makeColor(
							d["colors"]["charsheet_neutral_light_text"]["r"].GetInt(),
							d["colors"]["charsheet_neutral_light_text"]["g"].GetInt(),
							d["colors"]["charsheet_neutral_light_text"]["b"].GetInt(),
							d["colors"]["charsheet_neutral_light_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_neutral_lighter1_text") )
					{
						hudColors.characterSheetLighter1Neutral = makeColor(
							d["colors"]["charsheet_neutral_lighter1_text"]["r"].GetInt(),
							d["colors"]["charsheet_neutral_lighter1_text"]["g"].GetInt(),
							d["colors"]["charsheet_neutral_lighter1_text"]["b"].GetInt(),
							d["colors"]["charsheet_neutral_lighter1_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_neutral_darker1_text") )
					{
						hudColors.characterSheetDarker1Neutral = makeColor(
							d["colors"]["charsheet_neutral_darker1_text"]["r"].GetInt(),
							d["colors"]["charsheet_neutral_darker1_text"]["g"].GetInt(),
							d["colors"]["charsheet_neutral_darker1_text"]["b"].GetInt(),
							d["colors"]["charsheet_neutral_darker1_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_positive_text") )
					{
						hudColors.characterSheetGreen = makeColor(
							d["colors"]["charsheet_positive_text"]["r"].GetInt(),
							d["colors"]["charsheet_positive_text"]["g"].GetInt(),
							d["colors"]["charsheet_positive_text"]["b"].GetInt(),
							d["colors"]["charsheet_positive_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_negative_text") )
					{
						hudColors.characterSheetRed = makeColor(
							d["colors"]["charsheet_negative_text"]["r"].GetInt(),
							d["colors"]["charsheet_negative_text"]["g"].GetInt(),
							d["colors"]["charsheet_negative_text"]["b"].GetInt(),
							d["colors"]["charsheet_negative_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_faint_text") )
					{
						hudColors.characterSheetFaintText = makeColor(
							d["colors"]["charsheet_faint_text"]["r"].GetInt(),
							d["colors"]["charsheet_faint_text"]["g"].GetInt(),
							d["colors"]["charsheet_faint_text"]["b"].GetInt(),
							d["colors"]["charsheet_faint_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_off_white_text") )
					{
						hudColors.characterSheetOffWhiteText = makeColor(
							d["colors"]["charsheet_off_white_text"]["r"].GetInt(),
							d["colors"]["charsheet_off_white_text"]["g"].GetInt(),
							d["colors"]["charsheet_off_white_text"]["b"].GetInt(),
							d["colors"]["charsheet_off_white_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_heading_text") )
					{
						hudColors.characterSheetHeadingText = makeColor(
							d["colors"]["charsheet_heading_text"]["r"].GetInt(),
							d["colors"]["charsheet_heading_text"]["g"].GetInt(),
							d["colors"]["charsheet_heading_text"]["b"].GetInt(),
							d["colors"]["charsheet_heading_text"]["a"].GetInt());
					}
					if ( d["colors"].HasMember("charsheet_highlight_text") )
					{
						hudColors.characterSheetHighlightText = makeColor(
							d["colors"]["charsheet_highlight_text"]["r"].GetInt(),
							d["colors"]["charsheet_highlight_text"]["g"].GetInt(),
							d["colors"]["charsheet_highlight_text"]["b"].GetInt(),
							d["colors"]["charsheet_highlight_text"]["a"].GetInt());
					}
				}
				if ( d.HasMember("dropdowns") )
				{
					Player::GUIDropdown_t::allDropDowns.clear();
					for ( rapidjson::Value::ConstMemberIterator itr = d["dropdowns"].MemberBegin();
						itr != d["dropdowns"].MemberEnd(); ++itr )
					{
						auto& dropdown = Player::GUIDropdown_t::allDropDowns[itr->name.GetString()];
						dropdown.internalName = itr->name.GetString();
						if ( itr->value.HasMember("title") )
						{
							dropdown.title = itr->value["title"].GetString();
						}
						else
						{
							dropdown.title = language[4040]; // "interact"
						}
						if ( itr->value.HasMember("align_right") )
						{
							dropdown.alignRight = itr->value["align_right"].GetBool();
						}
						if ( itr->value.HasMember("gui_module") )
						{
							std::string moduleName = itr->value["gui_module"].GetString();
							if ( moduleName == "character_sheet" )
							{
								dropdown.module = Player::GUI_t::MODULE_CHARACTERSHEET;
							}
							else if ( moduleName == "inventory" )
							{
								dropdown.module = Player::GUI_t::MODULE_INVENTORY;
							}
							else if ( moduleName == "chest" )
							{
								dropdown.module = Player::GUI_t::MODULE_CHEST;
							}
							else if ( moduleName == "spells" )
							{
								dropdown.module = Player::GUI_t::MODULE_SPELLS;
							}
							else if ( moduleName == "hotbar" )
							{
								dropdown.module = Player::GUI_t::MODULE_HOTBAR;
							}
							else
							{
								dropdown.module = Player::GUI_t::MODULE_NONE;
							}
						}
						if ( itr->value.HasMember("default_option") )
						{
							dropdown.defaultOption = itr->value["default_option"].GetInt();
						}
						else
						{
							dropdown.defaultOption = 0;
						}
						for ( rapidjson::Value::ConstValueIterator options_itr = itr->value["options"].Begin();
							options_itr != itr->value["options"].End(); ++options_itr )
						{
							std::string text = "";
							std::string controller_glyph = "";
							std::string keyboard_glyph = "";
							std::string action = "no_action";
							if ( options_itr->HasMember("text") )
							{
								text = (*options_itr)["text"].GetString();
							}
							if ( options_itr->HasMember("controller_glyph") )
							{
								controller_glyph = (*options_itr)["controller_glyph"].GetString();
							}
							if ( options_itr->HasMember("keyboard_glyph") )
							{
								keyboard_glyph = (*options_itr)["keyboard_glyph"].GetString();
							}
							if ( options_itr->HasMember("action") )
							{
								action = (*options_itr)["action"].GetString();
							}
							dropdown.options.push_back(Player::GUIDropdown_t::DropdownOption_t(text, keyboard_glyph, controller_glyph, action));
						}
					}
				}
				printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
			}
		}
	}
}

const int kSpellListHeight = 294;
const int kSpellListGridY = 2;
void createPlayerSpellList(const int player)
{
	if ( !gui )
	{
		return;
	}

	if ( players[player]->inventoryUI.spellFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}

	Frame* frame = players[player]->inventoryUI.frame->addFrame("player spells");
	players[player]->inventoryUI.spellFrame = frame;
	frame->setSize(SDL_Rect{ 0,
		0,
		210,
		kSpellListHeight });
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);
	frame->setInheritParentFrameOpacity(false);

	SDL_Rect basePos{ 0, 0, 210, kSpellListHeight };
	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	players[player]->inventoryUI.spellSlotFrames.clear();

	const int baseSlotOffsetX = 18;
	const int baseSlotOffsetY = 36;
	const int baseGridOffsetY = kSpellListGridY;

	SDL_Rect invSlotsPos{ basePos.x + 4, basePos.y + 4, basePos.w, 242 };
	{
		int numGrids = (players[player]->inventoryUI.MAX_SPELLS_Y / players[player]->inventoryUI.spellPanel.kNumSpellsToDisplayVertical) + 1;

		const auto spellSlotsFrame = frame->addFrame("spell slots");
		spellSlotsFrame->setSize(invSlotsPos);
		spellSlotsFrame->setActualSize(SDL_Rect{ 0, 0, basePos.w, 242 * numGrids });
		spellSlotsFrame->setHollow(true);
		spellSlotsFrame->setAllowScrollBinds(false);

		auto gridImg = spellSlotsFrame->addImage(SDL_Rect{ baseSlotOffsetX, baseGridOffsetY, 162, 242 * numGrids },
			0xFFFFFFFF, "*images/ui/Inventory/HUD_Magic_ScrollGrid.png", "grid img");
		gridImg->tiled = true;

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };
		const int maxSpellsX = Player::Inventory_t::MAX_SPELLS_X;
		const int maxSpellsY = Player::Inventory_t::MAX_SPELLS_Y;

		for ( int x = 0; x < maxSpellsX; ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize);
			for ( int y = 0; y < maxSpellsY; ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + (y * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "spell %d %d", x, y);

				auto slotFrame = spellSlotsFrame->addFrame(slotname);
				players[player]->inventoryUI.spellSlotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}
	}

	{
		auto bgFrame = frame->addFrame("spell base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(true);
		const auto bgSize = bgFrame->getSize();
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, 210, kSpellListHeight },
			makeColor(255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Magic_Base.png", "spell base img");

		auto bgHeader = bgFrame->addImage(SDL_Rect{ 0, 0, 210, 22 },
			makeColor(255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Magic_TopHeader.png", "spell header img");
		//bg->disabled = false;

		auto slider = bgFrame->addSlider("spell slider");
		slider->setBorder(16);
		slider->setMinValue(0);
		slider->setMaxValue(100);
		slider->setValue(0);
		SDL_Rect sliderPos{ basePos.w - 26, 8, 20, 234 };
		slider->setRailSize(sliderPos);
		slider->setHandleSize(SDL_Rect{ 0, 0, 20, 28 });
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		//slider->setCallback(callback);
		slider->setColor(makeColor(255, 255, 255, 255));
		slider->setHighlightColor(makeColor(255, 255, 255, 255));
		slider->setHandleImage("*#images/ui/Sliders/HUD_Magic_Slider_Emerald_01.png");
		slider->setRailImage("*#images/ui/Sliders/HUD_Slider_Blank.png");
		slider->setHideGlyphs(true);
		slider->setHideKeyboardGlyphs(true);
		slider->setHideSelectors(true);
		slider->setMenuConfirmControlType(0);

		const char* font = "fonts/pixel_maz.ttf#32#2";
		auto titleText = bgFrame->addField("title txt", 64);
		titleText->setFont(font);
		titleText->setText("SPELLS");
		titleText->setHJustify(Field::justify_t::CENTER);
		titleText->setVJustify(Field::justify_t::TOP);
		titleText->setSize(SDL_Rect{ 56, 62, 96, 24 });
		titleText->setColor(makeColor(236, 175, 28, 255));

		auto closeBtn = bgFrame->addButton("close spell button");
		SDL_Rect closeBtnPos;
		closeBtnPos.x = 154;
		closeBtnPos.y = 60;
		closeBtnPos.w = 26;
		closeBtnPos.h = 26;
		closeBtn->setSize(closeBtnPos);
		closeBtn->setColor(makeColor(255, 255, 255, 255));
		closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
		closeBtn->setText("X");
		closeBtn->setFont(font);
		closeBtn->setHideGlyphs(true);
		closeBtn->setHideKeyboardGlyphs(true);
		closeBtn->setHideSelectors(true);
		closeBtn->setMenuConfirmControlType(0);
		closeBtn->setBackground("*#images/ui/Inventory/chests/Button_X_00.png");
		closeBtn->setBackgroundHighlighted("*#images/ui/Inventory/chests/Button_XHigh_00.png");
		closeBtn->setBackgroundActivated("*#images/ui/Inventory/chests/Button_XPress_00.png");

		closeBtn->setCallback([](Button& button) {
			messagePlayer(button.getOwner(), MESSAGE_DEBUG, "%d: Close spell button clicked", button.getOwner());
			if ( players[button.getOwner()]->inventory_mode == INVENTORY_MODE_SPELL )
			{
				players[button.getOwner()]->inventoryUI.cycleInventoryTab();
			}
			players[button.getOwner()]->inventoryUI.spellPanel.closeSpellPanel();
		});

		/*auto closeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
			makeColor(255, 255, 255, 255),
			"*#images/ui/Glyphs/Button_Xbox_DarkB_00.png", "close spell glyph");
		closeGlyph->pos.x = 154 + 10;
		closeGlyph->pos.y = 60 + 26;*/
	}
}

void closeChestGUIAction(const int player)
{
	players[player]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
	//players[player]->inventory_mode = INVENTORY_MODE_ITEM;
	if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_INVENTORY )
	{
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(player)->draw_cursor )
		{
			players[player]->GUI.warpControllerToModule(false);
		}
	}
	if ( openedChest[player] )
	{
		openedChest[player]->closeChest();
	}
	else
	{
		players[player]->inventoryUI.chestGUI.closeChest();
	}
}

bool takeAllChestGUIAction(const int player)
{
	if ( !openedChest[player] )
	{
		return false;
	}

	list_t* chest_inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player];
	}
	else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}
	if ( !chest_inventory )
	{
		// no chest inventory available
		return false;
	}

	std::vector<std::pair<int, Item*>> chestSlotOrder;
	for ( node_t* node = chest_inventory->first; node != nullptr; node = node->next )
	{
		Item* item2 = static_cast<Item*>(node->element);
		if ( item2 )
		{
			int key = item2->x + item2->y * 100;
			chestSlotOrder.push_back(std::make_pair(key, item2));
		}
	}
	int numItems = (int)chestSlotOrder.size();
	std::sort(chestSlotOrder.begin(), chestSlotOrder.end()); // sort ascending by position, left to right, then down
	int pickedUpItems = 0;
	for ( auto& keyValue : chestSlotOrder )
	{
		Item* item = keyValue.second;
		bool tryAddToInventory = true;
		int loops = 0;
		while ( tryAddToInventory )
		{
			++loops;
			if ( item->count <= 0 )
			{
				break;
			}
			int oldItemQty = 0;
			int destItemQty = 0;
			bool oldIdentify = item->identified;
			if ( skillCapstoneUnlocked(player, PRO_APPRAISAL) )
			{
				item->identified = true;
			}
			auto result = getItemStackingBehavior(player, item, nullptr, oldItemQty, destItemQty);
			item->identified = oldIdentify;
			int amountToPlace = item->count - oldItemQty;
			assert(amountToPlace > 0);
			if ( amountToPlace <= 0 )
			{
				break;
			}
			switch ( result.resultType )
			{
				case ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK:
				{
					if ( Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, result.itemToStackInto, false, false) )
					{
						// need to do another place operation.
					}
					else
					{
						tryAddToInventory = false;
					}
					if ( loops == 1 )
					{
						++pickedUpItems;
					}
					break;
				}
				case ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK:
				{
					// operation success, can finish here.
					Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, result.itemToStackInto, false, false);
					tryAddToInventory = false;
					if ( loops == 1 )
					{
						++pickedUpItems;
					}
					break;
				}
				case ITEM_ADDED_WITHOUT_NEEDING_STACK:
				{
					// check for inventory space
					if ( !players[player]->inventoryUI.bItemInventoryHasFreeSlot() )
					{
						// no space
						tryAddToInventory = false;
						break;
					}
					Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, nullptr, true, false);
					if ( oldItemQty > 0 )
					{
						// more work to do (unusually large stacks exceeding normal limits)
					}
					else
					{
						tryAddToInventory = false;
					}
					if ( loops == 1 )
					{
						++pickedUpItems;
					}
					break;
				}
				default:
					// error out
					tryAddToInventory = false;
					break;
			}
		}
	}

	if ( pickedUpItems > 0 )
	{
		messagePlayer(player, MESSAGE_INVENTORY, language[4099], pickedUpItems);
		playSound(35 + local_rng.rand() % 3, 64);
	}
	else if ( pickedUpItems == 0 && numItems > 0 )
	{
		messagePlayer(player, MESSAGE_INVENTORY, language[4100]);
	}

	return true;
}

const int chestBaseImgBorderWidth = 16;
const int chestBaseImgBorderTopHeight = 20;

void createChestGUI(const int player)
{
	if ( !gui )
	{
		return;
	}

	if ( players[player]->inventoryUI.chestFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}

	Frame* frame = players[player]->inventoryUI.frame->addFrame("chest");
	players[player]->inventoryUI.chestFrame = frame;

	frame->setSize(SDL_Rect{ 0,
		0,
		194,
		250 });
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);
	frame->setInheritParentFrameOpacity(false);

	SDL_Rect basePos{ 0, 0, 194, 130 };
	{
		auto bgFrame = frame->addFrame("chest base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(false);
		const auto bgSize = bgFrame->getSize();
		auto bg = bgFrame->addImage(SDL_Rect{ 6, 0, 182, 172 },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/chests/Chest_Main_00.png", "chest base img");
		auto bg2 = bgFrame->addImage(SDL_Rect{ 0, 0, 194, 66 },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/chests/Chest_Top_00.png", "chest lid img");
		//bg->disabled = false;

		//auto slider = bgFrame->addSlider("chest slider");
		//slider->setBorder(24);
		//slider->setMinValue(0);
		//slider->setMaxValue(100);
		//slider->setValue(0);
		//SDL_Rect sliderPos{ basePos.w - 38, 8, 30, 234 };
		//slider->setRailSize(sliderPos);
		//slider->setHandleSize(SDL_Rect{ 0, 0, 34, 34 });
		//slider->setOrientation(Slider::SLIDER_VERTICAL);
		////slider->setCallback(callback);
		//slider->setColor(makeColor(255, 255, 255, 255));
		//slider->setHighlightColor(makeColor(255, 255, 255, 255));
		//slider->setHandleImage("*#images/ui/Main Menus/Settings/Settings_Slider_Boulder00.png");
		//slider->setRailImage("*#images/ui/Main Menus/Settings/Settings_Slider_Backing00.png");
		//slider->setHideGlyphs(true);
		//slider->setHideKeyboardGlyphs(true);
		//slider->setHideSelectors(true);
		//slider->setMenuConfirmControlType(0);

		const char* font = "fonts/pixel_maz.ttf#32#2";
		auto titleText = bgFrame->addField("title txt", 64);
		titleText->setFont(font);
		titleText->setText("Chest");
		titleText->setHJustify(Field::justify_t::CENTER);
		titleText->setVJustify(Field::justify_t::CENTER);
		titleText->setSize(SDL_Rect{ basePos.x + 4, 0, 162, 32 });
		titleText->setColor(makeColor(188, 154, 114, 255));

		auto closeBtn = bgFrame->addButton("close chest button");
		SDL_Rect closeBtnPos = titleText->getSize();
		closeBtnPos.x = closeBtnPos.x + closeBtnPos.w - 98;
		closeBtnPos.w = 26;
		closeBtnPos.h = 26;
		closeBtn->setSize(closeBtnPos);
		closeBtn->setColor(makeColor(255, 255, 255, 255));
		closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
		closeBtn->setText("X");
		closeBtn->setFont(font);
		closeBtn->setHideGlyphs(true);
		closeBtn->setHideKeyboardGlyphs(true);
		closeBtn->setHideSelectors(true);
		closeBtn->setMenuConfirmControlType(0);
		closeBtn->setBackground("*#images/ui/Inventory/chests/Button_X_00.png");
		closeBtn->setBackgroundHighlighted("*#images/ui/Inventory/chests/Button_XHigh_00.png");
		closeBtn->setBackgroundActivated("*#images/ui/Inventory/chests/Button_XPress_00.png");
		closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
		closeBtn->setCallback([](Button& button) {
			closeChestGUIAction(button.getOwner());
		});

		auto grabAllBtn = bgFrame->addButton("grab all button");
		SDL_Rect grabBtnPos = titleText->getSize();
		grabBtnPos.x = closeBtnPos.x + closeBtnPos.w - 86;
		grabBtnPos.w = 86;
		grabBtnPos.h = 26;
		grabAllBtn->setSize(grabBtnPos);
		grabAllBtn->setColor(makeColor(255, 255, 255, 255));
		grabAllBtn->setHighlightColor(makeColor(255, 255, 255, 255));
		grabAllBtn->setText("Take All");
		grabAllBtn->setFont(font);
		grabAllBtn->setHideGlyphs(true);
		grabAllBtn->setHideKeyboardGlyphs(true);
		grabAllBtn->setHideSelectors(true);
		grabAllBtn->setMenuConfirmControlType(0);
		grabAllBtn->setBackground("*#images/ui/Inventory/chests/Button_TakeAll_00.png");
		grabAllBtn->setBackgroundHighlighted("*#images/ui/Inventory/chests/Button_TakeAllHigh_00.png");
		grabAllBtn->setBackgroundActivated("*#images/ui/Inventory/chests/Button_TakeAllPress_00.png");
		grabAllBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
		grabAllBtn->setCallback([](Button& button) {
			takeAllChestGUIAction(button.getOwner());
		});

		std::string promptFont = "fonts/pixel_maz.ttf#32#2";
		const int promptWidth = 60;
		const int promptHeight = 27;
		auto promptBack = bgFrame->addField("prompt back txt", 16);
		promptBack->setSize(SDL_Rect{ 0, 0, promptWidth, promptHeight });
		promptBack->setFont(promptFont.c_str());
		promptBack->setHJustify(Field::justify_t::RIGHT);
		promptBack->setVJustify(Field::justify_t::CENTER);
		promptBack->setText(language[4053]);
		//promptBack->setOntop(true);
		promptBack->setColor(makeColor(201, 162, 100, 255));

		auto promptBackImg = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
			"", "prompt back img");
		promptBackImg->disabled = true;

		auto promptGrabAll = bgFrame->addField("prompt grab txt", 16);
		promptGrabAll->setSize(SDL_Rect{ 0, 0, promptWidth, promptHeight });
		promptGrabAll->setFont(promptFont.c_str());
		promptGrabAll->setHJustify(Field::justify_t::RIGHT);
		promptGrabAll->setVJustify(Field::justify_t::CENTER);
		promptGrabAll->setText(language[4091]);
		//promptBack->setOntop(true);
		promptGrabAll->setColor(makeColor(201, 162, 100, 255));

		auto promptGrabImg = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
			"", "prompt grab img");
		promptBackImg->disabled = true;
	}

	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	players[player]->inventoryUI.chestSlotFrames.clear();

	const int baseSlotOffsetX = 0;
	const int baseSlotOffsetY = 0;

	const int gridHeight = 120 + 2; // 120px is grid img, plus 2px to tile the image for bottom border.
	SDL_Rect invSlotsPos{ basePos.x + chestBaseImgBorderWidth, basePos.y + 4 + chestBaseImgBorderTopHeight, basePos.w, gridHeight };
	{
		int numGrids = (players[player]->inventoryUI.MAX_CHEST_Y / players[player]->inventoryUI.chestGUI.kNumItemsToDisplayVertical) + 1;

		const auto chestSlotsFrame = frame->addFrame("chest slots");
		chestSlotsFrame->setSize(invSlotsPos);
		chestSlotsFrame->setActualSize(SDL_Rect{ 0, 0, basePos.w, gridHeight * numGrids });
		chestSlotsFrame->setHollow(true);
		chestSlotsFrame->setAllowScrollBinds(false);

		auto gridImg = chestSlotsFrame->addImage(SDL_Rect{ baseSlotOffsetX, baseSlotOffsetY, 162, gridHeight * numGrids },
			makeColor(255, 255, 255, 32), "*#images/ui/Inventory/HUD_Chest4x3_ScrollGrid.png", "grid img");
		gridImg->tiled = true;

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };
		const int maxChestX = Player::Inventory_t::MAX_CHEST_X;
		const int maxChestY = Player::Inventory_t::MAX_CHEST_Y;

		for ( int x = 0; x < maxChestX; ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize);
			for ( int y = 0; y < maxChestY; ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + (y * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "chest %d %d", x, y);

				auto slotFrame = chestSlotsFrame->addFrame(slotname);
				players[player]->inventoryUI.chestSlotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}
	}
}

const int Player::ShopGUI_t::MAX_SHOP_X = 5;
const int Player::ShopGUI_t::MAX_SHOP_Y = 5;

void closeShopGUIAction(const int player)
{
	players[player]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
	if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_INVENTORY )
	{
		players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(player)->draw_cursor )
		{
			players[player]->GUI.warpControllerToModule(false);
		}
	}
	if ( uidToEntity(shopkeeper[player]) )
	{
		closeShop(player);
	}
	else
	{
		players[player]->shopGUI.closeShop();
	}
}

void toggleShopBuybackView(const int player)
{
	players[player]->shopGUI.buybackView = !players[player]->shopGUI.buybackView;
}

void createShopGUI(const int player)
{
	if ( !gui )
	{
		return;
	}

	auto& shopGUI = players[player]->shopGUI;

	if ( shopGUI.shopFrame || !players[player]->inventoryUI.frame )
	{
		return;
	}

	SDL_Rect basePos{ 0, 0, 520, 334 };

	Frame* frame = players[player]->inventoryUI.frame->addFrame("shop");
	shopGUI.shopFrame = frame;
	frame->setSize(SDL_Rect{ players[player]->camera_virtualx1(),
		players[player]->camera_virtualy1(),
		basePos.w,
		basePos.h });
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);
	frame->setInheritParentFrameOpacity(false);

	{
		auto bgFrame = frame->addFrame("shop base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(false);
		const auto bgSize = bgFrame->getSize();
		auto bg = bgFrame->addImage(SDL_Rect{ 0, 0, basePos.w, basePos.h },
			makeColor(255, 255, 255, 255),
			"*#images/ui/Shop/Shop_Window_03C.png", "shop base img");
		auto bgGrid = bgFrame->addImage(SDL_Rect{ 12, 18, 206, 214},
			makeColor(255, 255, 255, 64),
			"*#images/ui/Shop/Shop_ItemSlots_Areas03.png", "shop grid img");

		auto bottomEdgeCover = bgFrame->addImage(SDL_Rect{ 0, basePos.h - 10, 316, 10 },
			makeColor(255, 255, 255, 255),
			"*#images/ui/Shop/Shop_BottomEdgeCover_00.png", "shop bottom edge img");
		bottomEdgeCover->ontop = true;

		const char* font = "fonts/pixel_maz_multiline.ttf#16#2";
		Uint32 titleColor = makeColor(219, 157, 20, 255);
		Uint32 titleOutline = makeColor(29, 16, 11, 255);
		auto titleText = bgFrame->addField("shop name", 64);
		titleText->setFont(font);
		titleText->setText("");
		titleText->setHJustify(Field::justify_t::CENTER);
		titleText->setVJustify(Field::justify_t::TOP);
		titleText->setSize(SDL_Rect{ 228, 29, 184, 24 });
		titleText->setTextColor(titleColor);
		titleText->setOutlineColor(titleOutline);

		const char* shoptypefont = "fonts/pixelmix.ttf#16#2";
		auto shopTypeText = bgFrame->addField("shop type", 64);
		shopTypeText->setFont(shoptypefont);
		shopTypeText->setText("");
		shopTypeText->setHJustify(Field::justify_t::CENTER);
		shopTypeText->setVJustify(Field::justify_t::TOP);
		shopTypeText->setSize(SDL_Rect{ 228, 48, 184, 82 });
		shopTypeText->setTextColor(titleColor);
		shopTypeText->setOutlineColor(titleOutline);

		auto shopkeeperImg = bgFrame->addImage(SDL_Rect{ basePos.w - 14 - 80, 14, 80, 80 }, 0xFFFFFFFF,
			"*#images/ui/Shop/shopkeeper.png", "shopkeeper img");

		auto closeBtn = bgFrame->addButton("close shop button");
		SDL_Rect closeBtnPos{ basePos.w - 34, 8, 26, 26 };
		closeBtn->setSize(closeBtnPos);
		closeBtn->setColor(makeColor(255, 255, 255, 255));
		closeBtn->setHighlightColor(makeColor(255, 255, 255, 255));
		closeBtn->setText("X");
		closeBtn->setFont(font);
		closeBtn->setHideGlyphs(true);
		closeBtn->setHideKeyboardGlyphs(true);
		closeBtn->setHideSelectors(true);
		closeBtn->setMenuConfirmControlType(0);
		closeBtn->setBackground("*#images/ui/Shop/Button_X_00.png");
		closeBtn->setBackgroundHighlighted("*#images/ui/Shop/Button_XHigh_00.png");
		closeBtn->setBackgroundActivated("*#images/ui/Shop/Button_XPress_00.png");
		closeBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
		closeBtn->setCallback([](Button& button) {
			closeShopGUIAction(button.getOwner());
		});

		auto buyTooltipFrame = bgFrame->addFrame("buy tooltip frame");
		buyTooltipFrame->setHollow(true);
		buyTooltipFrame->setBorder(0);
		buyTooltipFrame->setSize(SDL_Rect{ 4, basePos.h - 66, 310, 66 });
		buyTooltipFrame->setDisabled(true);

		auto itemTooltipImg = buyTooltipFrame->addImage(SDL_Rect{ 0, 0, 310, 66 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Shop_Tooltip_2Row_00.png", "tooltip img");

		auto itemGoldImg = buyTooltipFrame->addImage(SDL_Rect{ 0, 0, 20, 28 }, 0xFFFFFFFF,
			"*#images/ui/Inventory/tooltips/HUD_Tooltip_Icon_Money_00.png", "gold img");

		auto itemBgImg = buyTooltipFrame->addImage(SDL_Rect{ 0, 0, 54, 54 }, 0xFFFFFFFF,
			"*images/ui/Shop/Shop_Buy_BGSurround03.png", "item bg img");

		auto orbImg = buyTooltipFrame->addImage(SDL_Rect{ 210 - 8, 38, 16, 16 }, 0xFFFFFFFF,
			"", "orb img");
		orbImg->disabled = true;

		auto slotFrame = buyTooltipFrame->addFrame("item slot frame");
		SDL_Rect slotPos{ 0, 0, players[player]->inventoryUI.getSlotSize(), players[player]->inventoryUI.getSlotSize() };
		slotFrame->setSize(slotPos);
		slotFrame->setDisabled(true);
		createPlayerInventorySlotFrameElements(slotFrame);

		auto itemFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto itemNameText = buyTooltipFrame->addField("item display name", 1024);
		itemNameText->setFont(itemFont);
		itemNameText->setText("");
		itemNameText->setHJustify(Field::justify_t::LEFT);
		itemNameText->setVJustify(Field::justify_t::TOP);
		itemNameText->setSize(SDL_Rect{ 0, 0, 0, 0 });
		itemNameText->setColor(makeColor(201, 162, 100, 255));
		auto itemValueText = buyTooltipFrame->addField("item display value", 1024);
		itemValueText->setFont(itemFont);
		itemValueText->setText("");
		itemValueText->setHJustify(Field::justify_t::LEFT);
		itemValueText->setVJustify(Field::justify_t::TOP);
		itemValueText->setSize(SDL_Rect{ 0, 0, 0, 0 });
		itemValueText->setColor(makeColor(201, 162, 100, 255));

		auto buyPromptText = buyTooltipFrame->addField("buy prompt txt", 128);
		buyPromptText->setFont(itemFont);
		buyPromptText->setText("");
		buyPromptText->setHJustify(Field::justify_t::LEFT);
		buyPromptText->setVJustify(Field::justify_t::TOP);
		buyPromptText->setSize(SDL_Rect{ 0, 0, 0, 0 });
		buyPromptText->setColor(makeColor(255, 255, 255, 255));
		auto buyPromptFrame = buyTooltipFrame->addFrame("buy prompt frame");
		buyPromptFrame->setHollow(true);
		buyPromptFrame->setBorder(0);
		buyPromptFrame->setSize(SDL_Rect{0, 0, 0, 0});
		buyPromptFrame->setDisabled(true);
		auto buyPromptGlyph = buyPromptFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
			"", "buy prompt glyph");

		auto discountFrame = bgFrame->addFrame("discount frame");
		discountFrame->setHollow(true);
		discountFrame->setBorder(0);
		discountFrame->setSize(SDL_Rect{ bgFrame->getSize().w - 112, bgFrame->getSize().h - 124, 98, 24 });
		discountFrame->setDisabled(false);

		auto chatWindow = bgFrame->addFrame("chatter");
		auto tl = chatWindow->addImage(SDL_Rect{ 0, 0, 34, 34 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_TLWings00.png", "top left img");
		auto tm = chatWindow->addImage(SDL_Rect{ 0, 0, 2, 6 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_WingsT_00.png", "top img");
		auto tr = chatWindow->addImage(SDL_Rect{ 0, 0, 14, 28 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_TR00.png", "top right img");

		auto ml = chatWindow->addImage(SDL_Rect{ 0, 0, 6, 0 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_WingsL_00.png", "middle left img");
		auto mm1 = chatWindow->addImage(SDL_Rect{ 0, 0, 2, 2 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_CenterColor_00.png", "middle 1 img");
		auto mm2 = chatWindow->addImage(SDL_Rect{ 0, 0, 2, 2 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_CenterColor_00.png", "middle 2 img");
		auto mr = chatWindow->addImage(SDL_Rect{ 0, 0, 6, 0 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_WingsR_00.png", "middle right img");

		auto bl = chatWindow->addImage(SDL_Rect{ 0, 0, 28, 14 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_BL00.png", "bottom left img");
		auto bm = chatWindow->addImage(SDL_Rect{ 0, 0, 2, 6 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_WingsB_00.png", "bottom img");
		auto br = chatWindow->addImage(SDL_Rect{ 0, 0, 14, 14 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_BR00.png", "bottom right img");

		auto pointer = chatWindow->addImage(SDL_Rect{ 0, 0, 20, 22 }, 0xFFFFFFFF,
			"*#images/ui/Shop/Textbox_SpeakerPointer_TR01.png", "pointer img");

		auto bodyFont = "fonts/pixel_maz_multiline.ttf#16#2";
		auto chatText = chatWindow->addField("chat body", 1024);
		chatText->setFont(bodyFont);
		chatText->setText("");
		chatText->setHJustify(Field::justify_t::TOP);
		chatText->setVJustify(Field::justify_t::LEFT);
		chatText->setSize(SDL_Rect{ 0, 0, 0, 0 });
		//chatText->setColor(makeColor(29, 16, 11, 255));
		chatText->setTextColor(makeColor(29, 16, 11, 255));
		chatText->setOutlineColor(makeColor(0, 0, 0, 1));

		{
			auto valueFont = "fonts/pixel_maz.ttf#32#2";

			auto discountLabelText = bgFrame->addField("discount label", 32);
			discountLabelText->setFont(valueFont);
			discountLabelText->setText(language[4122]);
			discountLabelText->setHJustify(Field::justify_t::RIGHT);
			discountLabelText->setVJustify(Field::justify_t::TOP);
			discountLabelText->setSize(SDL_Rect{ bgFrame->getSize().w - 106 - 180 - 12, bgFrame->getSize().h - 92 - 30, 180, 24 });
			discountLabelText->setTextColor(makeColor(201, 162, 100, 255));
			discountLabelText->setOutlineColor(makeColor(29, 16, 11, 255));

			auto discountImg = discountFrame->addImage(SDL_Rect{ 0, 0, 98, 24 }, 0xFFFFFFFF,
				"*#images/ui/Shop/Shop_DiscountLabel_00.png", "discount img");

			auto discountValue = discountFrame->addField("discount", 32);
			discountValue->setFont(valueFont);
			discountValue->setText("");
			discountValue->setHJustify(Field::justify_t::RIGHT);
			discountValue->setVJustify(Field::justify_t::TOP);
			discountValue->setSize(SDL_Rect{ 14, 2, 80, 24 });
			discountValue->setColor(makeColor(201, 162, 100, 255));

			auto currentGoldText = bgFrame->addField("current gold", 32);
			currentGoldText->setFont(valueFont);
			currentGoldText->setText("");
			currentGoldText->setHJustify(Field::justify_t::RIGHT);
			currentGoldText->setVJustify(Field::justify_t::TOP);
			currentGoldText->setSize(SDL_Rect{ bgFrame->getSize().w - 80 - 18, bgFrame->getSize().h - 92, 80, 24 });
			currentGoldText->setColor(makeColor(201, 162, 100, 255));

			auto currentGoldLabelText = bgFrame->addField("current gold label", 32);
			currentGoldLabelText->setFont(valueFont);
			currentGoldLabelText->setText(language[4119]);
			currentGoldLabelText->setHJustify(Field::justify_t::RIGHT);
			currentGoldLabelText->setVJustify(Field::justify_t::TOP);
			currentGoldLabelText->setSize(SDL_Rect{ bgFrame->getSize().w - 106 - 100 - 12, bgFrame->getSize().h - 92, 100, 24 });
			currentGoldLabelText->setTextColor(makeColor(201, 162, 100, 255));
			currentGoldLabelText->setOutlineColor(makeColor(29, 16, 11, 255));

			auto changeGoldText = bgFrame->addField("change gold", 32);
			changeGoldText->setFont(valueFont);
			changeGoldText->setText("");
			changeGoldText->setHJustify(Field::justify_t::RIGHT);
			changeGoldText->setVJustify(Field::justify_t::TOP);
			changeGoldText->setSize(SDL_Rect{ bgFrame->getSize().w - 80 - 18, bgFrame->getSize().h - 92, 80, 24 });
			changeGoldText->setColor(makeColor(233, 220, 70, 255));
			changeGoldText->setOntop(true);
		}

		{
			auto promptFont = "fonts/pixel_maz.ttf#32#2";
			auto buybackText = bgFrame->addField("buyback txt", 32);
			buybackText->setFont(promptFont);
			buybackText->setText(language[4120]);
			buybackText->setHJustify(Field::justify_t::LEFT);
			buybackText->setVJustify(Field::justify_t::TOP);
			buybackText->setSize(SDL_Rect{ bgFrame->getSize().w - 178, bgFrame->getSize().h - 58, 178, 24 });
			buybackText->setTextColor(makeColor(201, 162, 100, 255));
			//buybackText->setOutlineColor(makeColor(29, 16, 11, 255));
			buybackText->setOutlineColor(makeColor(0, 0, 0, 255));

			auto buybackGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
				"", "buyback glyph");
			buybackGlyph->disabled = true;

			auto buybackBtn = bgFrame->addButton("buyback button");
			buybackBtn->setSize(SDL_Rect{ bgFrame->getSize().w - 208, bgFrame->getSize().h - 50, 194, 26 });
			buybackBtn->setFont(promptFont);
			buybackBtn->setText(language[4120]);
			buybackBtn->setColor(makeColor(255, 255, 255, 255));
			buybackBtn->setHighlightColor(makeColor(255, 255, 255, 255));
			buybackBtn->setHideGlyphs(true);
			buybackBtn->setHideKeyboardGlyphs(true);
			buybackBtn->setHideSelectors(true);
			buybackBtn->setMenuConfirmControlType(0);
			buybackBtn->setBackground("*#images/ui/Shop/Shop_Buyback_Button_00.png");
			buybackBtn->setBackgroundHighlighted("*#images/ui/Shop/Shop_Buyback_ButtonHigh_00.png");
			buybackBtn->setBackgroundActivated("*#images/ui/Shop/Shop_Buyback_ButtonPress_00.png");
			buybackBtn->setTextHighlightColor(makeColor(201, 162, 100, 255));
			buybackBtn->setCallback([](Button& button) {
				toggleShopBuybackView(button.getOwner());
			});

			auto closeText = bgFrame->addField("close shop prompt", 32);
			closeText->setFont(promptFont);
			closeText->setText(language[4121]);
			closeText->setHJustify(Field::justify_t::LEFT);
			closeText->setVJustify(Field::justify_t::TOP);
			closeText->setSize(SDL_Rect{ bgFrame->getSize().w - 178, bgFrame->getSize().h - 36, 178, 24 });
			closeText->setTextColor(makeColor(201, 162, 100, 255));
			//closeText->setOutlineColor(makeColor(29, 16, 11, 255));
			closeText->setOutlineColor(makeColor(0, 0, 0, 255));

			auto closeGlyph = bgFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF,
				"", "close shop glyph");
			closeGlyph->disabled = true;
			//closeText->setColor(makeColor(233, 220, 70, 255));
		}
	}

	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	shopGUI.shopSlotFrames.clear();

	const int baseSlotOffsetX = 0;
	const int baseSlotOffsetY = 0;

	SDL_Rect invSlotsPos{ 12, 16, 208, 216 };
	{
		const auto shopSlotsFrame = frame->addFrame("shop slots");
		shopSlotsFrame->setSize(invSlotsPos);
		shopSlotsFrame->setHollow(true);

		/*auto gridImg = chestSlotsFrame->addImage(SDL_Rect{ baseSlotOffsetX, baseSlotOffsetY, 162, gridHeight * numGrids },
			makeColor(255, 255, 255, 32), "*#images/ui/Inventory/HUD_Chest4x3_ScrollGrid.png", "grid img");
		gridImg->tiled = true;*/

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };
		const int maxShopX = Player::ShopGUI_t::MAX_SHOP_X;
		const int maxShopY = Player::ShopGUI_t::MAX_SHOP_Y;

		int accumulateSlotOffsetX = 0;
		for ( int x = 0; x < maxShopX; ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize) + accumulateSlotOffsetX;
			for ( int y = 0; y < maxShopY; ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + (y * (inventorySlotSize + 4));

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "shop %d %d", x, y);

				auto slotFrame = shopSlotsFrame->addFrame(slotname);
				shopGUI.shopSlotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);

				createPlayerInventorySlotFrameElements(slotFrame);
			}

			if ( x == 0 || x == 3 )
			{
				accumulateSlotOffsetX += 4;
			}
		}
	}
}

void createPlayerInventory(const int player)
{
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	Frame* frame = gameUIFrame[player]->addFrame(name);
	players[player]->inventoryUI.frame = frame;
	frame->setSize(SDL_Rect{ players[player]->camera_virtualx1(),
		players[player]->camera_virtualy1(),
		players[player]->camera_virtualWidth(),
		players[player]->camera_virtualHeight() });
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);
	frame->setInheritParentFrameOpacity(false);

	createChestGUI(player);
	createShopGUI(player);

	SDL_Rect basePos{ 0, 0, 210, 448 };
	{
		auto bgFrame = frame->addFrame("inventory base");
		bgFrame->setSize(basePos);
		bgFrame->setHollow(true);
		const auto bgSize = bgFrame->getSize();
		bgFrame->addImage(SDL_Rect{ 0, 0, bgSize.w, 448 },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Inventory_Base_02.png", "inventory base img");

		auto compactBase = bgFrame->addImage(SDL_Rect{ 0, 0, 210, 250 },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Inventory_BaseCompact_02.png", "inventory base compact img");
		compactBase->disabled = true;

		auto compactCharacterView = bgFrame->addImage(SDL_Rect{ 0, 0, 210, 214 },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Inventory_CharacterCompact_02.png", "inventory character compact img");
		compactCharacterView->disabled = true;

	}

	{
		auto backpackFrame = frame->addFrame("inventory backpack");
		backpackFrame->setSize(SDL_Rect{ 0, 202 + 242 - 2, 226, 102 });
		auto backpackImg = backpackFrame->addImage(SDL_Rect{ 0, 0, 226, 102 },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Inventory_Base_Bag_00a.png", "inventory backpack img");
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
			for ( int y = 0; y < players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY; ++y )
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

	SDL_Rect backpackSlotsPos{ 0, 202 + 242, basePos.w, 242 };
	{
		const auto backPackSlotsFrame = frame->addFrame("backpack slots");
		backPackSlotsFrame->setSize(backpackSlotsPos);
		const int backpackBaseSlotOffsetY = 8;
		int lowestY = 0;

		SDL_Rect currentSlotPos{ baseSlotOffsetX, backpackBaseSlotOffsetY, inventorySlotSize, inventorySlotSize };

		for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize);
			if ( x == 0 ) { currentSlotPos.x -= 4; } // backpack has unique first/last column entries
			if ( x == players[player]->inventoryUI.getSizeX() - 1 ) { currentSlotPos.x += 4; }

			for ( int y = players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY;
				y < players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY + players[player]->inventoryUI.getPlayerBackpackBonusSizeY(); ++y )
			{
				currentSlotPos.y = backpackBaseSlotOffsetY + ((y - players[player]->inventoryUI.DEFAULT_INVENTORY_SIZEY) * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);

				auto slotFrame = backPackSlotsFrame->addFrame(slotname);
				players[player]->inventoryUI.slotFrames[x + y * 100] = slotFrame;
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);
				lowestY = std::max(slotPos.y + slotPos.h, lowestY);
				//slotFrame->setDisabled(true);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}
		backpackSlotsPos.h = lowestY;
		backPackSlotsFrame->setSize(backpackSlotsPos);
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
				if ( players[player]->GUI.bActiveModuleUsesInventory() 
					&& players[player]->GUI.activeModule == Player::GUI_t::MODULE_INVENTORY )
				{
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
				}
				});
			charFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos) {
				drawCharacterPreview(widget.getOwner(), pos, 50, camera_charsheet_offsetyaw);
			});
			/*charFrame->addImage(SDL_Rect{ 0, 0, charSize.w, charSize.h },
				makeColor( 255, 255, 255, 255),
				"images/system/white.png", "inventory character preview bg");*/
		}

		/*auto selectedFrame = dollSlotsFrame->addFrame("paperdoll selected item");
		selectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		selectedFrame->setDisabled(true);

		Uint32 color = makeColor( 255, 255, 0, 255);
		selectedFrame->addImage(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "paperdoll selected highlight");*/
	}

	createPlayerSpellList(player);

	{
		auto selectedFrame = frame->addFrame("inventory selected item");
		selectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		selectedFrame->setDisabled(true);

		Uint32 color = makeColor( 255, 255, 0, 255);
		selectedFrame->addImage(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h },
			color, "*images/system/hotbar_slot.png", "inventory selected highlight");


		auto oldSelectedFrame = frame->addFrame("inventory old selected item");
		oldSelectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		oldSelectedFrame->setDisabled(true);

		const int itemSpriteSize = players[player]->inventoryUI.getItemSpriteSize();
		SDL_Rect itemSpriteBorder{ 2, 2, itemSpriteSize, itemSpriteSize };

		color = makeColor( 0, 255, 255, 255);
		auto oldImg = oldSelectedFrame->addImage(itemSpriteBorder,
			makeColor( 255, 255, 255, 128), "", "inventory old selected item");
		oldImg->disabled = true;
		oldSelectedFrame->addImage(SDL_Rect{ 0, 0, oldSelectedFrame->getSize().w, oldSelectedFrame->getSize().h },
			color, "*images/system/hotbar_slot.png", "inventory old selected highlight");

		auto bgFrame = frame->findFrame("inventory base");
		auto flourishFrame = frame->addFrame("inventory base flourish");
		flourishFrame->setSize(SDL_Rect{ (bgFrame->getSize().w / 2) - (122 / 2), 202 - 22 + 6, 122, 22 });
		auto flourishImg = flourishFrame->addImage(SDL_Rect{ 0, 0, flourishFrame->getSize().w, flourishFrame->getSize().h },
			makeColor( 255, 255, 255, 255),
			"*#images/ui/Inventory/HUD_Inventory_Flourish_00.png", "inventory flourish img");
		//flourishImg->disabled = true;

		GenericGUI[player].tinkerGUI.tinkerFrame = frame->addFrame("tinker");
		GenericGUI[player].tinkerGUI.tinkerFrame->setHollow(true);
		GenericGUI[player].tinkerGUI.tinkerFrame->setBorder(0);
		GenericGUI[player].tinkerGUI.tinkerFrame->setOwner(player);
		GenericGUI[player].tinkerGUI.tinkerFrame->setInheritParentFrameOpacity(false);
		GenericGUI[player].tinkerGUI.tinkerFrame->setDisabled(true);

		GenericGUI[player].alchemyGUI.alchFrame = frame->addFrame("alchemy");
		GenericGUI[player].alchemyGUI.alchFrame->setHollow(true);
		GenericGUI[player].alchemyGUI.alchFrame->setBorder(0);
		GenericGUI[player].alchemyGUI.alchFrame->setOwner(player);
		GenericGUI[player].alchemyGUI.alchFrame->setInheritParentFrameOpacity(false);
		GenericGUI[player].alchemyGUI.alchFrame->setDisabled(true);

		GenericGUI[player].featherGUI.featherFrame = frame->addFrame("feather");
		GenericGUI[player].featherGUI.featherFrame->setHollow(true);
		GenericGUI[player].featherGUI.featherFrame->setBorder(0);
		GenericGUI[player].featherGUI.featherFrame->setOwner(player);
		GenericGUI[player].featherGUI.featherFrame->setInheritParentFrameOpacity(false);
		GenericGUI[player].featherGUI.featherFrame->setDisabled(true);

		auto oldCursorFrame = frame->addFrame("inventory old item cursor");
		oldCursorFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize + 16, inventorySlotSize + 16 });
		oldCursorFrame->setDisabled(true);
		oldCursorFrame->setHollow(true);
		color = makeColor( 255, 255, 255, oldSelectedCursorOpacity);
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_TL.png", "inventory old cursor topleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_TR.png", "inventory old cursor topright");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_BL.png", "inventory old cursor bottomleft");
		oldCursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/SelectorGrey_BR.png", "inventory old cursor bottomright");

		auto cursorFrame = frame->addFrame("inventory selected item cursor");
		players[player]->inventoryUI.selectedItemCursorFrame = cursorFrame;
		cursorFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize + 16, inventorySlotSize + 16 });
		cursorFrame->setDisabled(true);
		cursorFrame->setHollow(true);
		color = makeColor( 255, 255, 255, selectedCursorOpacity);
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_TL.png", "inventory selected cursor topleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_TR.png", "inventory selected cursor topright");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_BL.png", "inventory selected cursor bottomleft");
		cursorFrame->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_BR.png", "inventory selected cursor bottomright");
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
	//messagePlayer(player.playernum, "%d %d", destx, desty);
}

void Player::Inventory_t::updateItemContextMenu()
{
	bool& toggleclick = inputs.getUIInteraction(player.playernum)->toggleclick;
	bool& itemMenuOpen = inputs.getUIInteraction(player.playernum)->itemMenuOpen;
	int& itemMenuSelected = inputs.getUIInteraction(player.playernum)->itemMenuSelected;
	Uint32& itemMenuItem = inputs.getUIInteraction(player.playernum)->itemMenuItem;
	int& itemMenuX = inputs.getUIInteraction(player.playernum)->itemMenuX;
	int& itemMenuY = inputs.getUIInteraction(player.playernum)->itemMenuY;
	bool& itemMenuFromHotbar = inputs.getUIInteraction(player.playernum)->itemMenuFromHotbar;
	int& itemMenuOffsetDetectionY = inputs.getUIInteraction(player.playernum)->itemMenuOffsetDetectionY;
	itemMenuOffsetDetectionY = 0;
	const Sint32 mousex = (inputs.getMouse(player.playernum, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	const Sint32 mousey = (inputs.getMouse(player.playernum, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;

	Item* item = uidToItem(itemMenuItem);
	if ( itemMenuItem != 0 && !item )
	{
		if ( chestGUI.bOpen && openedChest[player.playernum] )
		{
			list_t* chest_inventory = nullptr;
			if ( multiplayer == CLIENT )
			{
				chest_inventory = &chestInv[player.playernum];
			}
			else if ( openedChest[player.playernum]->children.first && openedChest[player.playernum]->children.first->element )
			{
				chest_inventory = (list_t*)openedChest[player.playernum]->children.first->element;
			}
			if ( chest_inventory )
			{
				node_t* nextnode = nullptr;
				for ( node_t* node = chest_inventory->first; node != NULL; node = nextnode )
				{
					nextnode = node->next;
					Item* chestItem = (Item*)node->element;
					if ( !chestItem ) { continue; }
					if ( chestItem->uid == itemMenuItem )
					{
						item = chestItem;
						break;
					}
				}
			}
		}
	}

	if ( !interactFrame )
	{
		itemMenuOpen = false;
		return;
	}

	auto highlightImageMid = interactFrame->findImage("interact selected highlight mid");
	highlightImageMid->disabled = true;
	highlightImageMid->color = hudColors.itemContextMenuOptionSelectedImg;
	auto highlightImageLeft = interactFrame->findImage("interact selected highlight left");
	highlightImageLeft->disabled = true;
	highlightImageLeft->color = hudColors.itemContextMenuOptionSelectedImg;
	auto highlightImageRight = interactFrame->findImage("interact selected highlight right");
	highlightImageRight->disabled = true;
	highlightImageRight->color = hudColors.itemContextMenuOptionSelectedImg;

	if ( !item || !itemMenuOpen )
	{
		itemMenuOpen = false;
		itemMenuSelected = 0;
		itemMenuFromHotbar = false;
		interactFrame->setDisabled(true);
		return;
	}

	interactFrame->setDisabled(false);

	auto options = getContextMenuOptionsForItem(player.playernum, item);
	if ( itemMenuFromHotbar )
	{
		options.push_back(PROMPT_CLEAR_HOTBAR_SLOT);
		std::reverse(options.begin(), options.end());
	}
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
				img->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuAlt1");
				break;
			case PROMPT_APPRAISE:
				img->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuAlt2");
				break;
			case PROMPT_DROP:
				img->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
				break;
			default:
				img->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuConfirm");
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

	//maxWidth += 16; -- edit this to add extra left/right padding

	const int rightClickProtectBuffer = (right_click_protect ? 0 : 10);

	bool alignRight = true;
	if ( itemMenuFromHotbar )
	{
		alignRight = true;
	}
	else if ( bCompactView )
	{
		if ( player.paperDoll.isItemOnDoll(*item) )
		{
			if ( paperDollPanelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
			{
				alignRight = false;
			}
			else
			{
				alignRight = true;
			}
		}
		else if ( itemCategory(item) == SPELL_CAT )
		{
			if ( spellPanel.panelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
			{
				alignRight = false;
			}
			else
			{
				alignRight = true;
			}
		}
		else
		{
			// normal inventory items
			if ( inventoryPanelJustify == PanelJustify_t::PANEL_JUSTIFY_RIGHT )
			{
				alignRight = false;
			}
			else
			{
				alignRight = true;
			}
		}
	}

	const int textStartX = (glyph1->disabled ? 0 : glyph1->pos.x + glyph1->pos.w) + textPaddingX;
	const int frameWidth = maxWidth + textStartX + textPaddingX;

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
		const int middleOffsetY = 13;
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

	if ( !alignRight )
	{
		frameSize.x -= frameSize.w;
	}

	if ( (frameSize.y + frameSize.h) > player.camera_virtualy2() )
	{
		int yoffset = (frameSize.y + frameSize.h) - player.camera_virtualy2();
		frameSize.y -= yoffset;
		itemMenuOffsetDetectionY = yoffset;
	}
	interactFrame->setSize(frameSize);

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
			absoluteSize.x -= (4 + ml->pos.w) + (alignRight ? rightClickProtectBuffer : 0);
			absoluteSize.w += ((4 + ml->pos.w) * 2 + rightClickProtectBuffer);
			absoluteSize.y += 4;
			absoluteSize.h -= 4;
			absoluteSize.y += itemMenuOffsetDetectionY;
			if ( mousex >= absoluteSize.x && mousex < absoluteSize.x + absoluteSize.w
				&& mousey >= absoluteSize.y && mousey < absoluteSize.y + absoluteSize.h )
			{
				itemMenuSelected = index;
			}
		}
		++index;
	}

	SDL_Rect absoluteSize = interactFrame->getAbsoluteSize();
	// right click protect uses exact border, else there is 10 px buffer
	absoluteSize.x -= (alignRight ? rightClickProtectBuffer : 0);
	absoluteSize.w += rightClickProtectBuffer;
	absoluteSize.y += itemMenuOffsetDetectionY;
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
		size.y += 3;
		highlightImageMid->pos = size;
		highlightImageMid->pos.x += highlightImageLeft->pos.w;
		highlightImageMid->pos.w -= 2 * highlightImageLeft->pos.w;
		highlightImageMid->pos.h = highlightImageLeft->pos.h;
		highlightImageMid->disabled = false;

		highlightImageLeft->pos.x = highlightImageMid->pos.x - highlightImageLeft->pos.w;
		highlightImageLeft->pos.y = highlightImageMid->pos.y;
		highlightImageLeft->disabled = false;
		highlightImageRight->pos.x = highlightImageMid->pos.x + highlightImageMid->pos.w;
		highlightImageRight->disabled = false;
		highlightImageRight->pos.y = highlightImageMid->pos.y;
	}

	bool activateSelection = false;
	if ( !Input::inputs[player.playernum].binary("MenuRightClick") && !toggleclick )
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
		interactFrame->setDisabled(true);
	}
}

void Player::Inventory_t::activateItemContextMenuOption(Item* item, ItemContextMenuPrompts prompt)
{
	if ( !item )
	{
		return;
	}
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

	bool sellingItemToShop = false;
	if ( players[player]->gui_mode == GUI_MODE_SHOP && itemCategory(item) != SPELL_CAT
		&& players[player]->shopGUI.bOpen && uidToEntity(shopkeeper[player]) )
	{
		sellingItemToShop = true;
	}

	if ( prompt == PROMPT_DROPDOWN )
	{
		return;
	}
	if ( prompt == PROMPT_CLEAR_HOTBAR_SLOT )
	{
		for ( auto& slot : players[player]->hotbar.slots() )
		{
			if ( slot.item == item->uid )
			{
				slot.item = 0;
			}
		}
		return;
	}
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
		if ( sellingItemToShop )
		{
			sellItemToShop(player, item);
		}
		return;
	}
	else if ( prompt == PROMPT_RETRIEVE_CHEST || prompt == PROMPT_RETRIEVE_CHEST_ALL )
	{
		bool emptiedSlot = false;
		if ( openedChest[player] )
		{
			if ( prompt == PROMPT_RETRIEVE_CHEST )
			{
				bool tryAddToInventory = true;
				while ( tryAddToInventory )
				{
					if ( item->count <= 0 )
					{
						break;
					}
					int oldItemQty = 0;
					int destItemQty = 0;
					int oldQty = item->count;
					item->count = 1;
					bool oldIdentify = item->identified;
					if ( skillCapstoneUnlocked(player, PRO_APPRAISAL) )
					{
						item->identified = true;
					}
					auto result = getItemStackingBehavior(player, item, nullptr, oldItemQty, destItemQty);
					item->identified = oldIdentify;
					item->count = oldQty;

					int amountToPlace = 1;
					switch ( result.resultType )
					{
						case ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK:
						case ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK:
						{
							// operation success, can finish here.
							Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, result.itemToStackInto, false);
							tryAddToInventory = false;
							if ( oldQty == 1 )
							{
								emptiedSlot = true;
							}
							break;
						}
						case ITEM_ADDED_WITHOUT_NEEDING_STACK:
						{
							// check for inventory space
							if ( !bItemInventoryHasFreeSlot() )
							{
								// no space
								tryAddToInventory = false;
								messagePlayer(player, MESSAGE_INVENTORY, language[727], item->getName()); // no room
								break;
							}

							// operation success, can finish here.
							int amountToPlace = 1;
							Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, nullptr, true);
							tryAddToInventory = false;
							if ( oldQty == 1 )
							{
								emptiedSlot = true;
							}
							break;
						}
						default:
							// error out
							tryAddToInventory = false;
							break;
					}
				}
			}
			else if ( prompt == PROMPT_RETRIEVE_CHEST_ALL )
			{
				bool tryAddToInventory = true;
				while ( tryAddToInventory )
				{
					if ( item->count <= 0 )
					{
						break;
					}
					int oldItemQty = 0;
					int destItemQty = 0;
					bool oldIdentify = item->identified;
					if ( skillCapstoneUnlocked(player, PRO_APPRAISAL) )
					{
						item->identified = true;
					}
					auto result = getItemStackingBehavior(player, item, nullptr, oldItemQty, destItemQty);
					item->identified = oldIdentify;
					int amountToPlace = item->count - oldItemQty;
					assert(amountToPlace > 0);
					if ( amountToPlace <= 0 )
					{
						break;
					}
					switch ( result.resultType )
					{
						case ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK:
						{
							if ( Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, result.itemToStackInto, false) )
							{
								// need to do another place operation.
							}
							else
							{
								tryAddToInventory = false;
							}
							break;
						}
						case ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK:
						{
							Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, result.itemToStackInto, false);
							if ( oldItemQty > 0 )
							{
								// more work to do (unusually large stacks exceeding normal limits)
							}
							else
							{
								// operation success, can finish here.
								tryAddToInventory = false;
								if ( oldItemQty == 0 )
								{
									emptiedSlot = true;
								}
							}
							break;
						}
						case ITEM_ADDED_WITHOUT_NEEDING_STACK:
						{
							// check for inventory space
							if ( !bItemInventoryHasFreeSlot() )
							{
								// no space
								tryAddToInventory = false;
								messagePlayer(player, MESSAGE_INVENTORY, language[727], item->getName()); // no room
								break;
							}
							Item* inventoryItem = takeItemFromChest(player, item, amountToPlace, nullptr, true);
							tryAddToInventory = false;
							if ( oldItemQty == 0 )
							{
								emptiedSlot = true;
							}
							break;
						}
						default:
							// error out
							tryAddToInventory = false;
							break;
					}
				}
			}
		}
		if ( !emptiedSlot && (inputs.getUIInteraction(player)->itemMenuOpen || players[player]->inventoryUI.bCompactView) )
		{
			tooltipDelayTick = ticks + TICKS_PER_SECOND / 2;
		}
		return;
	}
	else if ( prompt == PROMPT_STORE_CHEST || prompt == PROMPT_STORE_CHEST_ALL )
	{
		bool emptiedSlot = false;
		if ( !disableItemUsage || (disableItemUsage && !players[player]->paperDoll.isItemOnDoll(*item)) )
		{
			if ( openedChest[player] )
			{
				if ( prompt == PROMPT_STORE_CHEST )
				{
					bool tryAddToChest = true;
					while ( tryAddToChest )
					{
						if ( item->count <= 0 )
						{
							break;
						}
						int oldItemQty = 0;
						int destItemQty = 0;
						int oldQty = item->count;
						item->count = 1;
						auto result = getItemStackingBehaviorIntoChest(player, item, nullptr, oldItemQty, destItemQty);
						item->count = oldQty;
						
						int amountToPlace = 1;
						switch ( result.resultType )
						{
							case ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK:
							case ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK:
							{
								// operation success, can finish here.
								Item* itemInChest = openedChest[player]->addItemToChestFromInventory(player, item, amountToPlace, false, result.itemToStackInto);
								tryAddToChest = false;
								if ( oldQty == 1 )
								{
									emptiedSlot = true;
								}
								break;
							}
							case ITEM_ADDED_WITHOUT_NEEDING_STACK:
							{
								// check for chest space
								if ( numItemsInChest(player) + 1 > (players[player]->inventoryUI.MAX_CHEST_X * players[player]->inventoryUI.MAX_CHEST_Y) )
								{
									// no space
									tryAddToChest = false;
									messagePlayer(player, MESSAGE_INVENTORY, language[4098], item->getName()); // no room
									break;
								}

								// operation success, can finish here.
								int amountToPlace = 1;
								Item* itemInChest = openedChest[player]->addItemToChestFromInventory(player, item, amountToPlace, true, nullptr);
								tryAddToChest = false;
								if ( oldQty == 1 )
								{
									emptiedSlot = true;
								}
								break;
							}
							default:
								// error out
								tryAddToChest = false;
								break;
						}
					}
				}
				else if ( prompt == PROMPT_STORE_CHEST_ALL )
				{
					bool tryAddToChest = true;
					while ( tryAddToChest )
					{
						if ( item->count <= 0 )
						{
							break;
						}
						int oldItemQty = 0;
						int destItemQty = 0;
						auto result = getItemStackingBehaviorIntoChest(player, item, nullptr, oldItemQty, destItemQty);
						int amountToPlace = item->count - oldItemQty;
						assert(amountToPlace > 0);
						if ( amountToPlace <= 0 )
						{
							break;
						}
						switch ( result.resultType )
						{
							case ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK:
							{
								if ( Item* itemInChest = openedChest[player]->addItemToChestFromInventory(player, item, amountToPlace, false, result.itemToStackInto) )
								{
									// need to do another place operation.
								}
								else
								{
									tryAddToChest = false;
								}
								break;
							}
							case ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK:
							{
								// operation success, can finish here.
								Item* itemInChest = openedChest[player]->addItemToChestFromInventory(player, item, amountToPlace, false, result.itemToStackInto);
								tryAddToChest = false;
								if ( oldItemQty == 0 )
								{
									emptiedSlot = true;
								}
								break;
							}
							case ITEM_ADDED_WITHOUT_NEEDING_STACK:
							{
								// check for chest space
								if ( numItemsInChest(player) + 1 > (players[player]->inventoryUI.MAX_CHEST_X * players[player]->inventoryUI.MAX_CHEST_Y) )
								{
									// no space
									tryAddToChest = false;
									messagePlayer(player, MESSAGE_INVENTORY, language[4098], item->getName()); // no room
									break;
								}
								Item* itemInChest = openedChest[player]->addItemToChestFromInventory(player, item, amountToPlace, true, nullptr);
								if ( !itemInChest )
								{
									tryAddToChest = false; // failure adding to chest
								}
								else if ( oldItemQty > 0 )
								{
									// more work to do (unusually large stacks exceeding normal limits)
								}
								else
								{
									tryAddToChest = false;
									if ( oldItemQty == 0 )
									{
										emptiedSlot = true;
									}
								}
								break;
							}
							default:
								// error out
								tryAddToChest = false;
								break;
						}
					}
				}
			}
		}
		else
		{
			messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3432]); // unable to use in current form message.
		}
		if ( !emptiedSlot && (inputs.getUIInteraction(player)->itemMenuOpen || players[player]->inventoryUI.bCompactView) )
		{
			tooltipDelayTick = ticks + TICKS_PER_SECOND / 2;
		}
		return;
	}
	else if ( prompt == PROMPT_GRAB )
	{
		inputs.getUIInteraction(player)->selectedItemFromHotbar = -1;
		if ( this->player.inventoryUI.isItemFromChest(item) )
		{
			inputs.getUIInteraction(player)->selectedItemFromChest = item->uid;
		}
		inputs.getUIInteraction(player)->selectedItem = item;
		playSound(139, 64); // click sound
		inputs.getUIInteraction(player)->toggleclick = true;
		Input::inputs[player].consumeBinaryToggle("MenuLeftClick");
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
	else if ( prompt == PROMPT_CONSUME || prompt == PROMPT_CONSUME_ALTERNATE )
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
	else if ( prompt == PROMPT_INTERACT || prompt == PROMPT_INSPECT || prompt == PROMPT_INSPECT_ALTERNATE || prompt == PROMPT_TINKER )
	{
		if ( item->type == TOOL_ALEMBIC )
		{
			// not experimenting
			if ( GenericGUI[player].alchemyGUI.bOpen && GenericGUI[player].alembicItem == item )
			{
				GenericGUI[player].closeGUI();
			}
			else if ( !disableItemUsage )
			{
				GenericGUI[player].openGUI(GUI_TYPE_ALCHEMY, true, item);
			}
			else
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3432]); // unable to use in current form message.
			}
		}
		else if ( item->type == TOOL_TINKERING_KIT )
		{
			if ( !disableItemUsage )
			{
				if ( true /*item->status > BROKEN*/ ) // allow broken tinker kit
				{
					GenericGUI[player].openGUI(GUI_TYPE_TINKERING, item);
				}
				else
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, language[1092], item->getName()); // this is useless!
				}
			}
			else
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3432]); // unable to use in current form message.
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
		|| prompt == PROMPT_SPELL_EQUIP
		|| prompt == PROMPT_UNEQUIP_FOR_DROP )
	{
		if ( isItemEquippableInShieldSlot(item) && cast_animation[player].active_spellbook )
		{
			return; // don't try to equip shields while casting
		}

		if ( !disableItemUsage )
		{
			if ( item->status == BROKEN )
			{
				messagePlayer(player, MESSAGE_EQUIPMENT, language[1092], item->getName()); // don't try equip broken stuff
				return;
			}

			if ( prompt == PROMPT_EQUIP
				|| prompt == PROMPT_UNEQUIP
				|| prompt == PROMPT_UNEQUIP_FOR_DROP )
			{
				if ( items[item->type].item_slot == ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON
					|| itemCategory(item) == SPELLBOOK )
				{
					if ( prompt == PROMPT_UNEQUIP_FOR_DROP )
					{
						playerTryEquipItemAndUpdateServer(player, item, false); // don't check inventory space to unequip
					}
					else
					{
						playerTryEquipItemAndUpdateServer(player, item, true);
					}
					return;
				}
			}

			if ( prompt == PROMPT_UNEQUIP_FOR_DROP )
			{
				useItem(item, player, players[player]->entity, true);
			}
			else
			{
				useItem(item, player);
			}
			return;
		}
		else
		{
			if ( client_classes[player] == CLASS_SHAMAN && item->type == SPELL_ITEM )
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3488]); // unable to use with current level.
			}
			else
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3432]); // unable to use in current form message.
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
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3488]); // unable to use with current level.
			}
			else
			{
				messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_HINT | MESSAGE_EQUIPMENT, language[3432]); // unable to use in current form message.
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
	if ( !player.isLocalPlayer() )
	{
		return;
	}

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
	else
	{
		tooltipFrame->setOpacity(tooltipDisplay.opacitySetpoint);
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
	else
	{
		tooltipDisplay.expandCurrent = tooltipDisplay.expandSetpoint / 100.0;
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

		if ( players[player]->inventoryUI.appraisal.current_item == uid )
		{
			wasAppraisalTarget = true;
		}
		else
		{
			wasAppraisalTarget = false;
		}
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
		bool appraisingThisItem = false;
		if ( players[player]->inventoryUI.appraisal.current_item == uid )
		{
			appraisingThisItem = true;
		}

		if ( newItem->uid == uid
			&& newItem->type == type
			&& newItem->status == status
			&& newItem->beatitude == beatitude
			&& newItem->count == count
			&& newItem->appearance == appearance
			&& newItem->identified == identified
			&& (wasAppraisalTarget == appraisingThisItem)
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
	wasAppraisalTarget = false;
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

void Player::Inventory_t::updateItemContextMenuClickFrame()
{
	if ( !interactBlockClickFrame )
	{
		char interactBlockClickName[64] = "";
		snprintf(interactBlockClickName, sizeof(interactBlockClickName), "player inventory dropdown block click %d", player.playernum);
		if ( interactBlockClickFrame = gameUIFrame[player.playernum]->addFrame(interactBlockClickName) )
		{
			interactBlockClickFrame->setSize(SDL_Rect{ player.camera_virtualx1(),
				player.camera_virtualy1(),
				player.camera_virtualWidth(),
				player.camera_virtualHeight() });
			interactBlockClickFrame->setOwner(player.playernum);
			interactBlockClickFrame->setDisabled(true);
			interactBlockClickFrame->setHollow(true);
		}
	}

	interactBlockClickFrame->setSize(SDL_Rect{ player.camera_virtualx1(),
		player.camera_virtualy1(),
		player.camera_virtualWidth(),
		player.camera_virtualHeight() });

	if ( interactFrame )
	{
		if ( interactFrame->isDisabled() )
		{
			interactBlockClickFrame->setDisabled(true);
			interactBlockClickFrame->setHollow(true);
		}
		else
		{
			interactBlockClickFrame->setDisabled(false);
			interactBlockClickFrame->setHollow(false);
		}
	}
}

void Player::Inventory_t::updateCursor()
{
	if ( !frame )
	{
		return;
	}

	if ( !player.isLocalPlayer() )
	{
		return;
	}

	if ( cursor.queuedModule != Player::GUI_t::MODULE_NONE
		&& !player.shootmode && !nohud )
	{
		int cursorWidth = player.inventoryUI.getSlotSize();
		int cursorHeight = player.inventoryUI.getSlotSize();
		bool moveMouse = false;
		auto queuedModule = cursor.queuedModule;
		if ( cursor.queuedModule == Player::GUI_t::MODULE_INVENTORY )
		{
			if ( frame->isDisabled() || player.inventory_mode != INVENTORY_MODE_ITEM )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_INVENTORY )
		{
			if ( frame->isDisabled() || player.inventory_mode != INVENTORY_MODE_ITEM )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_SPELLS )
		{
			if ( spellFrame->isDisabled() || player.inventory_mode != INVENTORY_MODE_SPELL )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( spellPanel.isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_CHEST )
		{
			if ( !chestFrame 
				|| chestFrame->isDisabled()
				|| player.inventory_mode != INVENTORY_MODE_ITEM
				|| !openedChest[player.playernum] )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( chestGUI.isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_SHOP )
		{
			auto& shopGUI = player.shopGUI;
			if ( !shopGUI.shopFrame
				|| shopGUI.shopFrame->isDisabled()
				|| player.inventory_mode != INVENTORY_MODE_ITEM
				|| !shopGUI.bOpen )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( shopGUI.isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_TINKERING )
		{
			auto& tinkerGUI = GenericGUI[player.playernum].tinkerGUI;
			if ( !tinkerGUI.tinkerGUIHasBeenCreated()
				|| tinkerGUI.tinkerFrame->isDisabled()
				|| !tinkerGUI.isConstructMenuActive() )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( tinkerGUI.isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			if ( cursor.queuedFrameToWarpTo )
			{
				cursorWidth = cursor.queuedFrameToWarpTo->getSize().w;
				cursorHeight = cursor.queuedFrameToWarpTo->getSize().h;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_ALCHEMY )
		{
			auto& alchemyGUI = GenericGUI[player.playernum].alchemyGUI;
			if ( !alchemyGUI.alchemyGUIHasBeenCreated()
				|| alchemyGUI.alchFrame->isDisabled() )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( alchemyGUI.isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
		}
		else if ( cursor.queuedModule == Player::GUI_t::MODULE_FEATHER )
		{
			auto& featherGUI = GenericGUI[player.playernum].featherGUI;
			if ( !featherGUI.featherGUIHasBeenCreated()
				|| featherGUI.featherFrame->isDisabled() )
			{
				// cancel
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			else if ( featherGUI.isInteractable )
			{
				moveMouse = true;
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
			}
			if ( cursor.queuedFrameToWarpTo )
			{
				cursorWidth = cursor.queuedFrameToWarpTo->getSize().w;
				cursorHeight = cursor.queuedFrameToWarpTo->getSize().h;
			}
		}

		if ( moveMouse && cursor.queuedFrameToWarpTo )
		{
			//messagePlayer(0, "Queue warp: %d", queuedModule);
			cursor.queuedFrameToWarpTo->warpMouseToFrame(player.playernum, (Inputs::SET_CONTROLLER));
			SDL_Rect pos = cursor.queuedFrameToWarpTo->getAbsoluteSize();
			pos.x -= player.camera_virtualx1(); // offset any splitscreen camera positioning
			pos.y -= player.camera_virtualy1();
			player.inventoryUI.updateSelectedSlotAnimation(pos.x, pos.y,
				cursorWidth, cursorHeight, false);
			cursor.queuedFrameToWarpTo = nullptr;
		}
		else if ( cursor.queuedFrameToWarpTo && cursor.queuedModule == Player::GUI_t::MODULE_NONE )
		{
			//messagePlayer(0, "Queue cancel: %d", queuedModule);
		}
		else if ( cursor.queuedFrameToWarpTo )
		{
			//selectedItemCursorFrame->setDisabled(false); // show the cursor while we wait
		}
	}
	else
	{
		cursor.queuedFrameToWarpTo = nullptr;
		cursor.queuedModule = Player::GUI_t::MODULE_NONE;
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
					getColor(tl->color, &r, &g, &b, &a);
					a = oldSelectedCursorOpacity;
					tl->color = makeColor( r, g, b, a);
				}
				if ( auto tr = oldSelectedSlotCursor->findImage("inventory old cursor topright") )
				{
					tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
					tr->color = makeColor( r, g, b, a);
				}
				if ( auto bl = oldSelectedSlotCursor->findImage("inventory old cursor bottomleft") )
				{
					bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
					bl->color = makeColor( r, g, b, a);
				}
				if ( auto br = oldSelectedSlotCursor->findImage("inventory old cursor bottomright") )
				{
					br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
					br->color = makeColor( r, g, b, a);
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
			getColor(tl->color, &r, &g, &b, &a);
			a = selectedCursorOpacity;
			tl->color = makeColor( r, g, b, a);
		}
		if ( auto tr = selectedItemCursorFrame->findImage("inventory selected cursor topright") )
		{
			tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
			tr->color = makeColor( r, g, b, a);
		}
		if ( auto bl = selectedItemCursorFrame->findImage("inventory selected cursor bottomleft") )
		{
			bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
			bl->color = makeColor( r, g, b, a);
		}
		if ( auto br = selectedItemCursorFrame->findImage("inventory selected cursor bottomright") )
		{
			br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
			br->color = makeColor( r, g, b, a);
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
		cursorFrame = gameUIFrame[player.playernum]->addFrame(name);
		cursorFrame->setHollow(true);
		cursorFrame->setBorder(0);
		cursorFrame->setOwner(player.playernum);

		auto cursor = cursorFrame->addFrame("hud cursor");
		cursor->setHollow(true);
		cursor->setSize(SDL_Rect{ 0, 0, 0, 0 });
		Uint32 color = makeColor( 255, 255, 255, selectedCursorOpacity);
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_TL.png", "hud cursor topleft");
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_TR.png", "hud cursor topright");
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_BL.png", "hud cursor bottomleft");
		cursor->addImage(SDL_Rect{ 0, 0, 14, 14 },
			color, "*#images/ui/Inventory/Selector_BR.png", "hud cursor bottomright");
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
			if ( player.GUI.dropdownMenu.bOpen )
			{
				// animate cursor
			}
			else
			{
				offset = smallOffset; // don't animate while mouse normal hovering
			}
		}

		Uint8 r, g, b, a;
		if ( auto tl = hudCursor->findImage("hud cursor topleft") )
		{
			tl->pos = SDL_Rect{ offset, offset, tl->pos.w, tl->pos.h };
			getColor(tl->color, &r, &g, &b, &a);
			a = selectedCursorOpacity;
			tl->color = makeColor( r, g, b, a);
		}
		if ( auto tr = hudCursor->findImage("hud cursor topright") )
		{
			tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
			tr->color = makeColor( r, g, b, a);
		}
		if ( auto bl = hudCursor->findImage("hud cursor bottomleft") )
		{
			bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
			bl->color = makeColor( r, g, b, a);
		}
		if ( auto br = hudCursor->findImage("hud cursor bottomright") )
		{
			br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
			br->color = makeColor( r, g, b, a);
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

	if ( !player.isLocalPlayer() )
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
					getColor(tl->color, &r, &g, &b, &a);
					a = oldSelectedCursorOpacity;
					tl->color = makeColor( r, g, b, a);
				}
				if ( auto tr = oldSelectedSlotCursor->findImage("hotbar old cursor topright") )
				{
					tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
					tr->color = makeColor( r, g, b, a);
				}
				if ( auto bl = oldSelectedSlotCursor->findImage("hotbar old cursor bottomleft") )
				{
					bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
					bl->color = makeColor( r, g, b, a);
				}
				if ( auto br = oldSelectedSlotCursor->findImage("hotbar old cursor bottomright") )
				{
					br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
					br->color = makeColor( r, g, b, a);
				}
			}
		}
	}

	if ( auto selectedSlotCursor = hotbarFrame->findFrame("shootmode selected item cursor") )
	{
		SDL_Rect cursorSize = selectedSlotCursor->getSize();

		const int smallOffset = 2;
		const int largeOffset = 4;

		int offset = ((ticks - shootmodeCursor.lastUpdateTick) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2) ? largeOffset : smallOffset;
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
			getColor(tl->color, &r, &g, &b, &a);
			a = selectedCursorOpacity;
			tl->color = makeColor( r, g, b, a);
		}
		if ( auto tr = selectedSlotCursor->findImage("shootmode selected cursor topright") )
		{
			tr->pos = SDL_Rect{ -offset + cursorSize.w - tr->pos.w, offset, tr->pos.w, tr->pos.h };
			tr->color = makeColor( r, g, b, a);
		}
		if ( auto bl = selectedSlotCursor->findImage("shootmode selected cursor bottomleft") )
		{
			bl->pos = SDL_Rect{ offset, -offset + cursorSize.h - bl->pos.h, bl->pos.w, bl->pos.h };
			bl->color = makeColor( r, g, b, a);
		}
		if ( auto br = selectedSlotCursor->findImage("shootmode selected cursor bottomright") )
		{
			br->pos = SDL_Rect{ -offset + cursorSize.w - br->pos.w, -offset + cursorSize.h - br->pos.h, br->pos.w, br->pos.h };
			br->color = makeColor( r, g, b, a);
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
	if ( !player.characterSheet.sheetFrame )
	{
		player.characterSheet.createCharacterSheet();
	}
	if ( !frame )
	{
		createPlayerInventory(player.playernum);
	}
	if ( !tooltipFrame )
	{
		createInventoryTooltipFrame(player.playernum);
	}

	frame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight()});

	bool tooltipWasDisabled = tooltipFrame->isDisabled();

	updateInventory();

	player.shopGUI.updateShop();

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
		HPBar.flashAnimState = -1;
		HPBar.flashTicks = 0;
	}
	if ( mpFrame )
	{
		MPBar.animateSetpoint = stats[player.playernum]->MP;
		MPBar.animateValue = MPBar.animateSetpoint;
		MPBar.animateValue2 = MPBar.animateSetpoint;
		MPBar.animatePreviousSetpoint = MPBar.animateSetpoint;
		MPBar.animateState = ANIMATE_NONE;
		MPBar.flashAnimState = -1;
		MPBar.flashTicks = 0;
	}
}

void Player::HUD_t::updateGameTimer()
{
	if ( !gameTimerFrame )
	{
		return;
	}

	if ( !player.shootmode || !player.characterSheet.showGameTimerAlways || splitscreen )
	{
		gameTimerFrame->setDisabled(true);
		return;
	}

	gameTimerFrame->setDisabled(false);
	SDL_Rect pos = gameTimerFrame->getSize();
	pos.x = hudFrame->getSize().w - pos.w - 8 + 76;
	pos.y = 4;
	Field* timerText = gameTimerFrame->findField("timer txt");
	
	char buf[64] = "";
	Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
	Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
	Uint32 hour = (((completionTime / TICKS_PER_SECOND) / 60) / 60) % 24;
	Uint32 day = ((completionTime / TICKS_PER_SECOND) / 60) / 60 / 24;

	if ( day > 0 )
	{
		snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", day, hour, min, sec);
		pos.x -= 24;
	}
	else
	{
		snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, min, sec);
	}
	gameTimerFrame->setSize(pos);
	timerText->setText(buf);
}

void Player::HUD_t::updateXPBar()
{
	if ( !xpFrame )
	{
		return;
	}

	bool bCompact = false;
	if ( player.bUseCompactGUIWidth() || (keystatus[SDL_SCANCODE_T] && enableDebugKeys) )
	{
		bCompact = true;
	}

	xpFrame->setInheritParentFrameOpacity(false);
	xpFrame->setOpacity(100.0);
	SDL_Rect pos = xpFrame->getSize();
	pos.w = XP_FRAME_WIDTH + (bCompact ? xpbarCompactOffsetWidth : xpbarOffsetWidth);
	pos.x = hudFrame->getSize().w / 2 - pos.w / 2;
	if ( bCompact )
	{
		pos.y = 0 + (bCompact ? xpbarCompactOffsetY : xpbarOffsetY);
	}
	else
	{
		pos.y = 8 + (bCompact ? xpbarCompactOffsetY : xpbarOffsetY);
	}

	bool tempHideXP = false;
	if ( player.gui_mode == GUI_MODE_FOLLOWERMENU && player.bUseCompactGUIHeight() )
	{
		tempHideXP = true;
	}
	if ( tempHideXP )
	{
		//const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		//real_t setpointDiff = fpsScale * std::max(.1, (1.0 - animHideXP)) / 2.5;
		//animHideXP += setpointDiff;
		//animHideXP = std::min(1.0, animHideXP);
		animHideXP = 1.0;
	}
	else
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiff = fpsScale * std::max(.1, (animHideXP)) / 2.5;
		animHideXP -= setpointDiff;
		animHideXP = std::max(0.0, animHideXP);
	}
	//pos.y -= animHideXP * (pos.y + pos.h);
	xpFrame->setOpacity((1.0 - animHideXP) * 100.0);
	xpFrame->setSize(pos);

	auto xpBg = xpFrame->findImage("xp img base");
	auto xpProgress = xpFrame->findImage("xp img progress");
	xpProgress->disabled = true;
	auto xpProgressEndCap = xpFrame->findImage("xp img progress endcap");
	auto endCapRight = xpFrame->findImage("xp img endcap right");
	xpBg->pos.w = pos.w;
	if ( auto xpBgFlair = xpFrame->findImage("xp img base flair") )
	{
		xpBgFlair->pos.w = pos.w - 8;
	}
	endCapRight->pos.x = pos.w - endCapRight->pos.w;

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

			real_t setpointDiff = std::max(10.0, xpBar.animateSetpoint * 10.0 - xpBar.animateValue);
			real_t fpsScale = (144.f / std::max(1U, fpsLimit));
			xpBar.animateValue += fpsScale * (setpointDiff / 100.0); // reach it in x intervals, scaled to FPS
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
	SDL_Rect xpTextPos = xpText->getSize();
	xpTextPos.x = pos.w / 2 - (4 * 2) - xpTextPos.w;
	xpText->setSize(xpTextPos);
	auto xpTextStatic = xpFrame->findField("xp text static");
	SDL_Rect xpTextStaticPos = xpTextStatic->getSize();
	xpTextStaticPos.x = pos.w / 2 - 4;
	xpTextStatic->setSize(xpTextStaticPos);

	real_t percent = xpBar.animateValue / 1000.0;
	xpProgress->pos.w = std::max(1, static_cast<int>((xpBg->pos.w - xpProgressEndCap->pos.w) * percent));
	if ( !xpProgress->disabled )
	{
		xpProgressEndCap->pos.x = xpProgress->pos.x + xpProgress->pos.w;
	}

	auto xpProgressClipFrame = xpFrame->findFrame("xp progress clipping frame");
	SDL_Rect clipFramePos = xpProgressClipFrame->getSize();
	clipFramePos.w = std::max(1, static_cast<int>((xpBg->pos.w ) * percent));
	if ( xpProgress->disabled )
	{
		xpProgressEndCap->pos.x = clipFramePos.x + clipFramePos.w;

		bool animate = true;
		if ( ticks % 5 == 0 )
		{
			bool moving = (xpBar.animateSetpoint * 10.0 - xpBar.animateValue != 0);
			if ( moving && xpProgressEndCap->path == "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00.png" )
			{
				xpProgressEndCap->path = "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00a.png";
			}
			else if ( xpProgressEndCap->path == "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00a.png" )
			{
				xpProgressEndCap->path = "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00b.png";
			}
			else if ( xpProgressEndCap->path == "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00b.png" )
			{
				xpProgressEndCap->path = "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00c.png";
			}
			else if ( xpProgressEndCap->path == "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00c.png" )
			{
				xpProgressEndCap->path = "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00d.png";
			}
			else if ( xpProgressEndCap->path == "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00d.png" )
			{
				xpProgressEndCap->path = "*#images/ui/HUD/xpbar/HUD_Exp_SandCap_00.png";
			}
		}
	}
	xpProgressClipFrame->setSize(clipFramePos);
	auto xpProgressClipFrameImg = xpProgressClipFrame->findImage("xp img progress clipped");
	xpProgressClipFrameImg->pos.x = -(xpProgressClipFrameImg->pos.w - pos.w) / 2;
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
		getColor(img->color, &r, &g, &b, &a);
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
		getColor(img->color, &r, &g, &b, &a);
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
		getColor(img->color, &r, &g, &b, &a);
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
		getColor(img->color, &r, &g, &b, &a);
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
		getColor(txt->getColor(), &r, &g, &b, &a);
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
				if ( StatusEffectQueue_t::StatusEffectDefinitions_t::effectDefinitionExists(i) )
				{
					int variation = -1;
					SDL_Surface* srcSurf = nullptr;
					if ( i == EFF_SHAPESHIFT )
					{
						if ( entity && entity->behavior == &actPlayer )
						{
							switch ( entity->effectShapeshift )
							{
								case RAT:
									variation = 0;
									break;
								case SPIDER:
									variation = 1;
									break;
								case TROLL:
									variation = 2;
									break;
								case CREATURE_IMP:
									variation = 3;
									break;
								default:
									break;
							}
						}
					}
					auto& definition = StatusEffectQueue_t::StatusEffectDefinitions_t::getEffect(i);
					if ( !definition.neverDisplay )
					{
						std::string imgPath = StatusEffectQueue_t::StatusEffectDefinitions_t::getEffectImgPath(definition, variation);
						if ( imgPath != "" )
						{
							srcSurf = const_cast<SDL_Surface*>(Image::get(imgPath.c_str())->getSurf());

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
		}
	}
	if ( enemy_statusEffects2 != 0 )
	{
		for ( int i = 0; i < 32; ++i )
		{
			if ( (enemy_statusEffects2 & (1 << i)) != 0 )
			{
				int effectID = i + 32;
				if ( StatusEffectQueue_t::StatusEffectDefinitions_t::effectDefinitionExists(effectID) )
				{
					int variation = -1;
					SDL_Surface* srcSurf = nullptr;
					if ( i == EFF_SHAPESHIFT )
					{
						if ( entity && entity->behavior == &actPlayer )
						{
							switch ( entity->effectShapeshift )
							{
								case RAT:
									variation = 0;
									break;
								case SPIDER:
									variation = 1;
									break;
								case TROLL:
									variation = 2;
									break;
								case CREATURE_IMP:
									variation = 3;
									break;
								default:
									break;
							}
						}
					}
					auto& definition = StatusEffectQueue_t::StatusEffectDefinitions_t::getEffect(effectID);
					if ( !definition.neverDisplay )
					{
						std::string imgPath = StatusEffectQueue_t::StatusEffectDefinitions_t::getEffectImgPath(definition, variation);
						if ( imgPath != "" )
						{
							srcSurf = const_cast<SDL_Surface*>(Image::get(imgPath.c_str())->getSurf());

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
	//if ( bIsMostRecentHPBar && !enemyDetails->hasDistanceCheck )
	//{
	//	//enemyDetails->hasDistanceCheck = true;
	//	auto& camera = cameras[player.playernum];
	//	double playerdist = sqrt(pow(camera.x * 16.0 - enemyDetails->worldX, 2) + pow(camera.y * 16.0 - enemyDetails->worldY, 2));
	//	if ( playerdist >= 3 * 16.0 )
	//	{
	//		//enemyDetails->displayOnHUD = true;
	//	}
	//}

	SDL_Rect pos = whichFrame->getSize();
	bool doFadeout = false;
	bool doAnimation = true;
	if ( enemyDetails->displayOnHUD && whichFrame == enemyBarFrameHUD )
	{
		doAnimation = false;
	}

	if ( doAnimation && !enemyDetails->displayOnHUD && !enemyDetails->expired && enemyDetails->animator.setpoint <= 0 )
	{
		if ( ticks - enemyDetails->enemy_timer >= EnemyHPDamageBarHandler::shortDistanceHPBarFadeTicks )
		{
			auto& camera = cameras[player.playernum];
			double playerdist = sqrt(pow(camera.x * 16.0 - enemyDetails->worldX, 2) + pow(camera.y * 16.0 - enemyDetails->worldY, 2));
			if ( playerdist <= EnemyHPDamageBarHandler::shortDistanceHPBarFadeDistance * 16.0 )
			{
				enemyDetails->expired = true;
			}
		}
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
		getColor(skull.first->color, &r, &g, &b, &a);
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
		skull.first->color = makeColor( r, g, b, a);
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
			getColor(dmgText->getColor(), &r, &g, &b, &a);
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
				getColor(dmgText->getColor(), &r, &g, &b, &a);
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
		getColor(skull.first->color, &r, &g, &b, &a);

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
		skull.first->color = makeColor( r, g, b, a);
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
			getColor(dmgText->getColor(), &r, &g, &b, &a);
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
				getColor(dmgText->getColor(), &r, &g, &b, &a);
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

const int HPMPdividerThresholdInterval = 20;

void Player::HUD_t::updateHPBar()
{
	if ( !hpFrame )
	{
		return;
	}

	bool bCompact = false;
	if ( player.bUseCompactGUIWidth() || (keystatus[SDL_SCANCODE_T] && enableDebugKeys) )
	{
		bCompact = true;
	}

	SDL_Rect pos = hpFrame->getSize();
	pos.w = HPMP_FRAME_WIDTH + (bCompact ? hpmpbarCompactOffsetWidth : hpmpbarOffsetWidth);
	pos.x = HPMP_FRAME_START_X + (bCompact ? hpmpbarCompactOffsetX : hpmpbarOffsetX);
	pos.y = hudFrame->getSize().h - HPMP_FRAME_START_Y + (bCompact ? hpmpbarCompactOffsetY : hpmpbarOffsetY);
	hpFrame->setSize(pos);

	auto hpForegroundFrame = hpFrame->findFrame("hp foreground frame");
	{
		auto _pos = hpForegroundFrame->getSize();
		_pos.w = pos.w;
		hpForegroundFrame->setSize(_pos);
	}
	auto hpBg = hpFrame->findImage("hp img base");
	auto hpEndcap = hpForegroundFrame->findImage("hp img endcap");
	auto hpProgressBot = hpForegroundFrame->findImage("hp img progress bot");
	auto hpProgress = hpForegroundFrame->findImage("hp img progress");
	auto hpProgressEndCap = hpForegroundFrame->findImage("hp img progress endcap");
	auto hpFadeFrame = hpFrame->findFrame("hp fade frame");
	{
		auto _pos = hpFadeFrame->getSize();
		_pos.w = pos.w;
		hpFadeFrame->setSize(_pos);
	}
	auto hpFadedBase = hpFadeFrame->findImage("hp img fade bot");
	auto hpFaded = hpFadeFrame->findImage("hp img fade");
	auto hpFadedEndCap = hpFadeFrame->findImage("hp img fade endcap");

	real_t progressWidth = hpFrame->getSize().w - 74;
	int backgroundWidth = hpFrame->getSize().w - 54;

	// handle bar size changing
	{
		real_t multiplier = 1.0;
		const Sint32 maxHPWidth = (bCompact ? hpmpbarCompactMaxWidthAmount : hpmpbarMaxWidthAmount);
		if ( stats[player.playernum]->MAXHP < maxHPWidth )
		{
			// start at 30%, increase 2.5% every 5 HP past 20 MAXHP
			multiplier = (bCompact ? hpmpbarCompactBasePercentSize : hpmpbarBasePercentSize) / 100.0;
			real_t widthIntervalPercent = (bCompact ? hpmpbarCompactWidthIncreasePercentOnInterval : hpmpbarWidthIncreasePercentOnInterval) / 100.0;
			int intervalThreshold = (bCompact ? hpmpbarCompactIntervalToIncreaseWidth : hpmpbarIntervalToIncreaseWidth);
			int baseIntervalStart = (bCompact ? hpmpbarCompactIntervalStartValue : hpmpbarIntervalStartValue);
			multiplier += (widthIntervalPercent * ((std::max(0, stats[player.playernum]->MAXHP - baseIntervalStart) / intervalThreshold)));
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

		// flash for taking damage
		HPBar.flashTicks = ticks;
		HPBar.flashAnimState = -1;
		HPBar.flashType = FLASH_ON_DAMAGE;
	}

	if ( HPBar.maxValue > stats[player.playernum]->MAXHP )
	{
		hpFadedValue = HPBar.animateSetpoint; // resetting game etc, stop fade animation sticking out of frame
	}

	HPBar.maxValue = stats[player.playernum]->MAXHP;

	if ( hpForegroundValue < HPBar.animateSetpoint ) // gaining HP, animate
	{
		// flash for gaining HP, provided not already flashing
		/*if ( HPBar.flashAnimState == -1 || (HPBar.flashAnimState >= 0 && HPBar.flashType != FLASH_ON_DAMAGE) )
		{
			HPBar.flashTicks = ticks;
			HPBar.flashAnimState = -1;
			HPBar.flashType = FLASH_ON_RECOVERY;
		}*/

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

	// dividers
	{
		const int fullBarWidth = hpProgressBot->pos.w + progressWidth + hpEndcap->pos.w / 2;
		auto div25Percent = hpForegroundFrame->findImage("hp img div 25pc");
		div25Percent->disabled = false;
		div25Percent->pos.x = hpProgressBot->pos.x + fullBarWidth * .25 - 2;
		auto div50Percent = hpForegroundFrame->findImage("hp img div 50pc");
		div50Percent->disabled = false;
		div50Percent->pos.x = hpProgressBot->pos.x + fullBarWidth * .5 - 2;
		auto div75Percent = hpForegroundFrame->findImage("hp img div 75pc");
		div75Percent->disabled = false;
		div75Percent->pos.x = hpProgressBot->pos.x + fullBarWidth * .75 - 2;

		if ( div50Percent->pos.x - div25Percent->pos.x < HPMPdividerThresholdInterval )
		{
			div75Percent->disabled = true;
			// 2 dividers 33%/66%
			div25Percent->pos.x = hpProgressBot->pos.x + fullBarWidth * .33 - 2;
			div50Percent->pos.x = hpProgressBot->pos.x + fullBarWidth * .66 - 2;
			if ( div50Percent->pos.x - div25Percent->pos.x < HPMPdividerThresholdInterval )
			{
				// 1 divider 50%
				div25Percent->disabled = true;
				div50Percent->pos.x = hpProgressBot->pos.x + fullBarWidth * .5 - 2;
			}
		}
	}

	hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_00.png";
	hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_00.png";
	hpProgressEndCap->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_00.png";
	auto hpProgressEndCapFlash = hpForegroundFrame->findImage("hp img progress endcap flash");
	hpProgressEndCapFlash->disabled = true;
	const int framesPerAnimation = HPBar.flashType == FLASH_ON_DAMAGE ? 1 : 2;
	const int numAnimationFrames = HPBar.flashType == FLASH_ON_DAMAGE ? 20 : 2;
	if ( HPBar.flashTicks > 0 )
	{
		//messagePlayer(0, MESSAGE_DEBUG, "ticks: %d, animticks: %d, state: %d", ticks, HPBarFlashTicks, HPBarFlashAnimState);
		if ( HPBar.flashAnimState > numAnimationFrames || hpProgress->disabled )
		{
			HPBar.flashTicks = 0;
			HPBar.flashType = FLASH_ON_DAMAGE;
			HPBar.flashAnimState = -1;
			hpProgressEndCapFlash->disabled = true;
		}
		else
		{
			hpProgressEndCapFlash->disabled = hpProgressEndCap->disabled;
			if ( ticks == HPBar.flashTicks )
			{
				HPBar.flashAnimState = 1;
				HPBar.flashProcessedOnTick = ticks;
			}
			else if ( (HPBar.flashProcessedOnTick != ticks)
				&& (ticks > HPBar.flashTicks) 
				&& (ticks - HPBar.flashTicks) % framesPerAnimation == 0 )
			{
				++HPBar.flashAnimState;
				HPBar.flashProcessedOnTick = ticks;
			}

			if ( HPBar.flashType == 0 )
			{
				if ( HPBar.flashAnimState <= 6 )
				{
					hpProgressEndCapFlash->color = 0xFFFFFFFF;
				}

				if ( HPBar.flashAnimState == 0 )
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_00.png";
					hpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F00.png";
					hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_00.png";
				}
				else if ( HPBar.flashAnimState >= 1 && HPBar.flashAnimState <= 2 )
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_01.png";
					hpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F01.png";
					hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_01.png";
				}
				else if ( HPBar.flashAnimState == 3 )
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_02.png";
					hpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F02.png";
					hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_02.png";
				}
				else if ( HPBar.flashAnimState >= 4 && HPBar.flashAnimState <= 5 )
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_01.png";
					hpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F01.png";
					hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_01.png";
				}
				else if ( HPBar.flashAnimState == 6 )
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_03.png";
					hpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F03.png";
					hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_03.png";
				}
				else
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_00.png";
					hpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_F00.png";
					Uint8 r, g, b, a;
					getColor(hpProgressEndCapFlash->color, &r, &g, &b, &a);
					int decrement = 20;
					real_t fpsScale = (60.f / std::max(1U, fpsLimit));
					decrement *= fpsScale;
					a = std::max(0, (int)a - decrement);
					hpProgressEndCapFlash->color = makeColor(r, g, b, a);
				}
			}
			else
			{
				hpProgressEndCapFlash->color = 0xFFFFFFFF;
				hpProgressEndCapFlash->disabled = true;
				if ( HPBar.flashAnimState == 1 )
				{
					hpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPMid_03.png";
					hpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPBot_03.png";
					hpProgressEndCap->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_HPEnd_01.png";
				}
			}
		}
	}
	else
	{
		HPBar.flashAnimState = -1;
		hpProgressEndCapFlash->disabled = true;
	}
	{
		hpProgressEndCapFlash->pos.w = 22;
		hpProgressEndCapFlash->section.x = 0;
		hpProgressEndCapFlash->section.w = 0;
		if ( !hpProgressEndCap->disabled )
		{
			hpProgressEndCapFlash->pos.x = hpProgressEndCap->pos.x - (hpProgressEndCapFlash->pos.w - hpProgressEndCap->pos.w);
		}
		if ( hpProgressEndCapFlash->pos.x < hpProgressBot->pos.x )
		{
			// adjust end cap flash to clip correctly sliding past end of bar
			int overflowx = (hpProgressBot->pos.x - hpProgressEndCapFlash->pos.x);
			hpProgressEndCapFlash->section.x = (overflowx);
			hpProgressEndCapFlash->pos.x += overflowx;
			hpProgressEndCapFlash->pos.w -= overflowx;
			hpProgressEndCapFlash->section.w = hpProgressEndCapFlash->pos.w;
		}
	}
}

void Player::HUD_t::updateMPBar()
{
	if ( !mpFrame )
	{
		return;
	}

	bool bCompact = false;
	if ( player.bUseCompactGUIWidth() || (keystatus[SDL_SCANCODE_T] && enableDebugKeys) )
	{
		bCompact = true;
	}

	SDL_Rect pos = mpFrame->getSize();
	pos.w = HPMP_FRAME_WIDTH + (bCompact ? hpmpbarCompactOffsetWidth : hpmpbarOffsetWidth);
	pos.x = HPMP_FRAME_START_X + (bCompact ? hpmpbarCompactOffsetX : hpmpbarOffsetX);
	pos.y = hudFrame->getSize().h - HPMP_FRAME_START_Y + HPMP_FRAME_HEIGHT + (bCompact ? hpmpbarCompactOffsetY : hpmpbarOffsetY);
	mpFrame->setSize(pos);

	auto mpForegroundFrame = mpFrame->findFrame("mp foreground frame");
	{
		auto _pos = mpForegroundFrame->getSize();
		_pos.w = pos.w;
		mpForegroundFrame->setSize(_pos);
	}
	auto mpBg = mpFrame->findImage("mp img base");
	auto mpEndcap = mpForegroundFrame->findImage("mp img endcap");
	auto mpProgressBot = mpForegroundFrame->findImage("mp img progress bot");
	auto mpProgress = mpForegroundFrame->findImage("mp img progress");
	auto mpProgressEndCap = mpForegroundFrame->findImage("mp img progress endcap");
	auto mpFadeFrame = mpFrame->findFrame("mp fade frame");
	{
		auto _pos = mpFadeFrame->getSize();
		_pos.w = pos.w;
		mpFadeFrame->setSize(_pos);
	}
	auto mpFadedBase = mpFadeFrame->findImage("mp img fade bot");
	auto mpFaded = mpFadeFrame->findImage("mp img fade");
	auto mpFadedEndCap = mpFadeFrame->findImage("mp img fade endcap");

	real_t progressWidth = mpFrame->getSize().w - 74;
	int backgroundWidth = mpFrame->getSize().w - 54;

	// handle bar size changing
	{
		real_t multiplier = 1.0;
		const Sint32 maxMPWidth = (bCompact ? hpmpbarCompactMaxWidthAmount : hpmpbarMaxWidthAmount);
		if ( stats[player.playernum]->MAXMP < maxMPWidth )
		{
			// start at 30%, increase 2.5% every 5 MP past 20 MAXMP
			multiplier = (bCompact ? hpmpbarCompactBasePercentSize : hpmpbarBasePercentSize) / 100.0;
			real_t widthIntervalPercent = (bCompact ? hpmpbarCompactWidthIncreasePercentOnInterval : hpmpbarWidthIncreasePercentOnInterval) / 100.0;
			int intervalThreshold = (bCompact ? hpmpbarCompactIntervalToIncreaseWidth : hpmpbarIntervalToIncreaseWidth);
			int baseIntervalStart = (bCompact ? hpmpbarCompactIntervalStartValue : hpmpbarIntervalStartValue);
			multiplier += (widthIntervalPercent * ((std::max(0, stats[player.playernum]->MAXMP - baseIntervalStart) / intervalThreshold)));
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

	bool flashAnimationPreviouslyPlaying = MPBar.flashTicks > 0;
	if ( MPBar.animateSetpoint < MPBar.animatePreviousSetpoint ) // insta-change as losing health
	{
		mpForegroundValue = MPBar.animateSetpoint;
		MPBar.animateTicks = ticks;

		// flash for taking damage
		MPBar.flashTicks = ticks;
		MPBar.flashProcessedOnTick = 0;
		MPBar.flashAnimState = -1;
		MPBar.flashType = FLASH_ON_DAMAGE;
	}

	if ( MPBar.maxValue > stats[player.playernum]->MAXMP )
	{
		mpFadedValue = MPBar.animateSetpoint; // resetting game etc, stop fade animation sticking out of frame
	}

	MPBar.maxValue = stats[player.playernum]->MAXMP;
	if ( mpForegroundValue < MPBar.animateSetpoint ) // gaining MP, animate
	{
		// flash for gaining MP, provided not already flashing
		/*if ( MPBar.flashAnimState == -1 || (MPBar.flashAnimState >= 0 && MPBar.flashType != FLASH_ON_DAMAGE) )
		{
			MPBar.flashTicks = ticks;
			MPBar.flashAnimState = -1;
			MPBar.flashType = FLASH_ON_RECOVERY;
		}*/

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

	// dividers
	{
		const int fullBarWidth = mpProgressBot->pos.w + progressWidth + mpEndcap->pos.w / 2;
		auto div25Percent = mpForegroundFrame->findImage("mp img div 25pc");
		div25Percent->disabled = false;
		div25Percent->pos.x = mpProgressBot->pos.x + fullBarWidth * .25 - 2;
		auto div50Percent = mpForegroundFrame->findImage("mp img div 50pc");
		div50Percent->disabled = false;
		div50Percent->pos.x = mpProgressBot->pos.x + fullBarWidth * .5 - 2;
		auto div75Percent = mpForegroundFrame->findImage("mp img div 75pc");
		div75Percent->disabled = false;
		div75Percent->pos.x = mpProgressBot->pos.x + fullBarWidth * .75 - 2;

		if ( div50Percent->pos.x - div25Percent->pos.x < HPMPdividerThresholdInterval )
		{
			div75Percent->disabled = true;
			// 2 dividers 33%/66%
			div25Percent->pos.x = mpProgressBot->pos.x + fullBarWidth * .33 - 2;
			div50Percent->pos.x = mpProgressBot->pos.x + fullBarWidth * .66 - 2;
			if ( div50Percent->pos.x - div25Percent->pos.x < HPMPdividerThresholdInterval )
			{
				// 1 divider 50%
				div25Percent->disabled = true;
				div50Percent->pos.x = mpProgressBot->pos.x + fullBarWidth * .5 - 2;
			}
		}
	}

	mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_00.png";
	mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_00.png";
	mpProgressEndCap->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_00.png";
	auto mpProgressEndCapFlash = mpForegroundFrame->findImage("mp img progress endcap flash");
	mpProgressEndCapFlash->disabled = true;
	const int framesPerAnimation = MPBar.flashType == FLASH_ON_DAMAGE ? 1 : 2;
	const int numAnimationFrames = MPBar.flashType == FLASH_ON_DAMAGE ? 30 : 2;
	if ( MPBar.flashTicks > 0 )
	{
		if ( MPBar.flashAnimState > numAnimationFrames || mpProgress->disabled )
		{
			MPBar.flashTicks = 0;
			MPBar.flashType = FLASH_ON_DAMAGE;
			MPBar.flashAnimState = -1;
			mpProgressEndCapFlash->disabled = true;
		}
		else
		{
			mpProgressEndCapFlash->disabled = mpProgressEndCap->disabled;
			bool processedOnTick = MPBar.flashProcessedOnTick == ticks;
			if ( ticks == MPBar.flashTicks )
			{
				MPBar.flashAnimState = 1;
				MPBar.flashProcessedOnTick = ticks;
			}
			else if ( (!processedOnTick)
				&& (ticks > MPBar.flashTicks)
				&& (ticks - MPBar.flashTicks) % framesPerAnimation == 0 )
			{
				++MPBar.flashAnimState;
				MPBar.flashProcessedOnTick = ticks;
			}

			if ( MPBar.flashType == 0 )
			{
				if ( MPBar.flashAnimState <= 16 && MPBar.flashAnimState >= 10 )
				{
					mpProgressEndCapFlash->color = 0xFFFFFFFF;
				}
				else if ( MPBar.flashAnimState == 0 )
				{
					mpProgressEndCapFlash->color = makeColor(255, 255, 255, 0);
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00.png";
				}

				if ( MPBar.flashAnimState <= 9 )
				{
					if ( MPBar.flashAnimState == 7 )
					{
						// we need the MP bar to flash long enough for long spellcast times
						// can adjust how many animStates we skip here to play with timing,
						// without changing other state machine code
						MPBar.flashAnimState = 9; // 1, 2 skip a few..
					}
					Uint8 r, g, b, a;
					getColor(mpProgressEndCapFlash->color, &r, &g, &b, &a);
					int increment = 10;
					real_t fpsScale = (60.f / std::max(1U, fpsLimit));
					increment *= fpsScale;
					a = std::min(255, (int)a + increment);
					mpProgressEndCapFlash->color = makeColor(r, g, b, a);

					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_00.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_00.png";

					if ( MPBar.flashAnimState % 2 == 0
						&& !processedOnTick )
					{
						if ( mpProgressEndCapFlash->path == "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00.png" )
						{
							mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00b.png";
						}
						else if ( mpProgressEndCapFlash->path == "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00b.png" )
						{
							mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00c.png";
						}
						else if ( mpProgressEndCapFlash->path == "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00c.png" )
						{
							mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00d.png";
						}
						else 
						{
							mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00.png";
						}
					}
				}
				else if ( MPBar.flashAnimState <= 10 )
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_00.png";
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_00.png";
				}
				else if ( MPBar.flashAnimState >= 11 && MPBar.flashAnimState <= 12 )
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_01.png";
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F01.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_01.png";
				}
				else if ( MPBar.flashAnimState == 13 )
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_02.png";
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F02.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_02.png";
				}
				else if ( MPBar.flashAnimState >= 14 && MPBar.flashAnimState <= 15 )
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_01.png";
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F01.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_01.png";
				}
				else if ( MPBar.flashAnimState == 16 )
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_03.png";
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F03.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_03.png";
				}
				else
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_00.png";
					mpProgressEndCapFlash->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_F00.png";
					Uint8 r, g, b, a;
					getColor(mpProgressEndCapFlash->color, &r, &g, &b, &a);
					int decrement = 20;
					real_t fpsScale = (60.f / std::max(1U, fpsLimit));
					decrement *= fpsScale;
					a = std::max(0, (int)a - decrement);
					mpProgressEndCapFlash->color = makeColor(r, g, b, a);
				}
			}
			else
			{
				mpProgressEndCapFlash->color = 0xFFFFFFFF;
				mpProgressEndCapFlash->disabled = true;
				if ( MPBar.flashAnimState == 1 )
				{
					mpProgress->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPMid_03.png";
					mpProgressBot->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPBot_03.png";
					mpProgressEndCap->path = "*#images/ui/HUD/hpmpbars/HUD_Bars_MPEnd_01.png";
				}
			}
		}
	}
	else
	{
		MPBar.flashAnimState = -1;
		mpProgressEndCapFlash->disabled = true;
	}

	{
		mpProgressEndCapFlash->pos.w = 22;
		mpProgressEndCapFlash->section.x = 0;
		mpProgressEndCapFlash->section.w = 0;
		if ( !mpProgressEndCap->disabled )
		{
			mpProgressEndCapFlash->pos.x = mpProgressEndCap->pos.x - (mpProgressEndCapFlash->pos.w - mpProgressEndCap->pos.w);
		}
		if ( mpProgressEndCapFlash->pos.x < mpProgressBot->pos.x )
		{
			// adjust end cap flash to clip correctly sliding past end of bar
			int overflowx = (mpProgressBot->pos.x - mpProgressEndCapFlash->pos.x);
			mpProgressEndCapFlash->section.x = (overflowx);
			mpProgressEndCapFlash->pos.x += overflowx;
			mpProgressEndCapFlash->pos.w -= overflowx;
			mpProgressEndCapFlash->section.w = mpProgressEndCapFlash->pos.w;
		}
	}
}

void Player::Hotbar_t::updateHotbar()
{
	if ( !hotbarFrame )
	{
		return;
	}

	bool tempHideHotbar = false;
	if ( player.gui_mode == GUI_MODE_FOLLOWERMENU && player.bUseCompactGUIHeight() )
	{
		tempHideHotbar = true;
	}
	if ( tempHideHotbar )
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiff = fpsScale * std::max(.1, (1.0 - animHide)) / 2.5;
		animHide += setpointDiff;
		animHide = std::min(1.0, animHide);
	}
	else
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiff = fpsScale * std::max(.1, (animHide)) / 2.5;
		animHide -= setpointDiff;
		animHide = std::max(0.0, animHide);
	}

	bool bCompactView = false;
	if ( (keystatus[SDL_SCANCODE_U] && enableDebugKeys) || player.bUseCompactGUIWidth() )
	{
		bCompactView = true;
	}
	int hotbarStartY1 = hotbarFrame->getSize().h + getHotbarStartY1(); // higher row (center group)
	int hotbarStartY2 = hotbarFrame->getSize().h + getHotbarStartY2(); // lower row (left/right)
	hotbarStartY1 += (bCompactView ? hotbarCompactOffsetY : hotbarOffsetY);
	hotbarStartY2 += (bCompactView ? hotbarCompactOffsetY : hotbarOffsetY);
	const int hotbarCentreX = hotbarFrame->getSize().w / 2;
	const int hotbarCentreXLeft = hotbarCentreX - 148 + (bCompactView ? hotbarCompactOffsetX : 0);
	const int hotbarCentreXRight = hotbarCentreX + 148 - (bCompactView ? hotbarCompactOffsetX : 0);
	hotbarStartY1 += animHide * abs(getHotbarStartY1());
	hotbarStartY2 += animHide * abs(getHotbarStartY1());

	if ( !player.shootmode || FollowerMenu[player.playernum].followerMenuIsOpen() )
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
	if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_NONE || (player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR && !player.shootmode) )
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

	auto cancelPromptTxt = hotbarFrame->findField("hotbar cancel prompt");
	cancelPromptTxt->setDisabled(true);
	auto cancelPromptGlyph = hotbarFrame->findImage("hotbar cancel glyph");
	cancelPromptGlyph->disabled = true;

	if ( !bCompactView && useHotbarFaceMenu && faceMenuButtonHeld != FaceMenuGroup::GROUP_NONE )
	{
		cancelPromptTxt->setDisabled(false);
		cancelPromptTxt->setText(language[3063]);
		cancelPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarCancel");
		if ( auto imgGet = Image::get(cancelPromptGlyph->path.c_str()) )
		{
			cancelPromptGlyph->pos.w = imgGet->getWidth();
			cancelPromptGlyph->pos.h = imgGet->getHeight();
			cancelPromptGlyph->disabled = false;
		}
		SDL_Rect promptTxtPos = cancelPromptTxt->getSize();
		promptTxtPos.x = hotbarCentreX;
		promptTxtPos.y = hotbarStartY1 + getSlotSize();
		if ( auto textGet = cancelPromptTxt->getTextObject() )
		{
			promptTxtPos.x -= textGet->getWidth() / 2;
			promptTxtPos.x += cancelPromptGlyph->pos.w / 2 + 2;
			if ( promptTxtPos.x % 2 == 1 )
			{
				++promptTxtPos.x;
			}
		}
		cancelPromptGlyph->pos.y = promptTxtPos.y;
		cancelPromptGlyph->pos.x = promptTxtPos.x - 4 - cancelPromptGlyph->pos.w;
		cancelPromptTxt->setSize(promptTxtPos);
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

		auto slot = getHotbarSlotFrame(num);
		assert(slot);

		if ( auto img = slot->findImage("slot img") ) // apply any opacity from config
		{
			Uint8 r, g, b, a;
			getColor(img->color, &r, &g, &b, &a);
			a = hotbarSlotOpacity;
			img->color = makeColor( r, g, b, a);
		}
		if ( highlightSlotImg )
		{
			Uint8 r, g, b, a;
			getColor(highlightSlotImg->color, &r, &g, &b, &a);
			a = hotbarSelectedSlotOpacity;
			highlightSlotImg->color = makeColor( r, g, b, a);
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
		int compactViewOffset = 0;
		int compactInactiveSlotOffset = 0;
		int compactExpandedOffsetX = 0;
		int centreXLeft = hotbarCentreXLeft;
		int centreXRight = hotbarCentreXRight;
		int centreX = hotbarCentreX;
		bool compactViewNavigation = (player.GUI.activeModule == Player::GUI_t::MODULE_HOTBAR && !player.shootmode);
		if ( bCompactView )
		{
			if ( compactViewNavigation )
			{
				compactViewOffset = pos.w * hotbarCompactSlotOverlapPercent / 2.0;

				compactExpandedOffsetX = hotbarCompactExpandedOffsetX;
				centreXLeft -= compactExpandedOffsetX;
				centreXRight += compactExpandedOffsetX;
			}
			else
			{
				compactViewOffset = pos.w * hotbarCompactSlotOverlapPercent;
			}
			compactInactiveSlotOffset = hotbarCompactInactiveSlotMovementX * selectedSlotAnimateCurrentValue;

			if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_NONE )
			{
				if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_LEFT )
				{
					centreX += compactInactiveSlotOffset;
					centreXRight += compactInactiveSlotOffset;
				}
				else if( faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE )
				{
					centreXLeft -= compactInactiveSlotOffset;
					centreXRight += compactInactiveSlotOffset;
				}
				else if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_RIGHT )
				{
					centreXLeft -= compactInactiveSlotOffset;
					centreX -= compactInactiveSlotOffset;
				}
			}
		}
		pos.x = centreX;
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
				if ( !player.shootmode )
				{
					glyph->disabled = true;
				}
			}

			slot->findField("slot num text")->setDisabled(true); // disable the hotkey prompts per slot
			switch ( num )
			{
				// left group
				case 0:
					pos.x = centreXLeft - pos.w / 2 - pos.w + 2 + compactViewOffset;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_LEFT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
						pos.x -= compactViewOffset * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarModifierLeft");
					break;
				case 1:
					pos.x = centreXLeft - pos.w / 2;
					pos.y -= slotYMovement;
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarLeft");
					break;
				case 2:
					pos.x = centreXLeft + (pos.w / 2 - 2) - compactViewOffset;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_LEFT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
						pos.x += compactViewOffset * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarModifierRight");
					break;
				// middle group
				case 3:
					pos.y = hotbarStartY1;
					pos.x = centreX - pos.w / 2 - pos.w + 2 + compactViewOffset;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
						pos.x -= compactViewOffset * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarModifierLeft");
					break;
				case 4:
					pos.y = hotbarStartY1;
					pos.y -= slotYMovement;
					pos.x = centreX - pos.w / 2;
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarUp");
					break;
				case 5:
					pos.y = hotbarStartY1;
					pos.x = centreX + (pos.w / 2 - 2) - compactViewOffset;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
						pos.x += compactViewOffset * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarModifierRight");
					break;
				// right group
				case 6:
					pos.x = centreXRight - pos.w / 2 - pos.w + 2 + compactViewOffset;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_RIGHT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
						pos.x -= compactViewOffset * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarModifierLeft");
					break;
				case 7:
					pos.x = centreXRight - pos.w / 2;
					pos.y -= slotYMovement;
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarRight");
					break;
				case 8:
					pos.x = centreXRight + (pos.w / 2 - 2) - compactViewOffset;
					if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_RIGHT )
					{
						pos.y -= slotYMovement * selectedSlotAnimateCurrentValue;
						pos.x += compactViewOffset * selectedSlotAnimateCurrentValue;
					}
					else
					{
						glyph->disabled = true;
					}
					glyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("HotbarFacebarModifierRight");
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
						if ( inputs.getUIInteraction(player.playernum)->itemMenuOpen &&
							inputs.getUIInteraction(player.playernum)->itemMenuFromHotbar )
						{
							showCursor = true;
						}
						else if ( inputs.getUIInteraction(player.playernum)->selectedItem 
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
								player.inventoryUI.updateSelectedSlotAnimation(pos.x - 1, pos.y - 1, getSlotSize() - 2, getSlotSize() - 2, 
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
								updateSelectedSlotAnimation(pos.x - 1, pos.y - 1, getSlotSize() - 2, getSlotSize() - 2, snapCursor);
							}
						}
					}
					else
					{
						highlightSlotImg->disabled = true;
						highlightSlotItem->setDisabled(true);
						updateSlotFrameFromItem(slotItem, uidToItem(hotbar[num].item));
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

	return hotbarSlotFrames[hotbarSlot];
}

static void drawConsoleCommandBuffer() {
    if (!command) {
        return;
    }
	int commandPlayer = clientnum;
	for ( int i = 0; i < MAXPLAYERS; ++i ) {
		if ( inputs.bPlayerUsingKeyboardControl(i) ) {
			commandPlayer = i;
			break;
		}
	}
	char buf[1024];
	if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 ) {
	    snprintf(buf, sizeof(buf), "> %s_", command_str);
	} else {
	    snprintf(buf, sizeof(buf), "> %s", command_str);
	}
	auto text = Text::get(buf, "fonts/pixelmix.ttf#16#2",
	    0xffffffff, makeColor(0, 0, 0, 255));
	const int printx = players[commandPlayer]->camera_virtualx1() + 8;
	const int printy = players[commandPlayer]->camera_virtualy2() - 192;
	text->draw(SDL_Rect{0,0,0,0}, SDL_Rect{printx, printy, 0, 0},
	    SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
}

static Uint32 gui_ticks = 0u;
Frame::result_t doFrames() {
    Frame::result_t result;
    result.usable = false;
    result.highlightTime = 0;
    result.tooltip = nullptr;
    result.removed = false;
	if ( gui )
	{
		while ( gui_ticks < ticks )
		{
			++gui_ticks;
		}

        static ConsoleVariable<bool> gui_process("/gui_process", true);
        if (*gui_process) {
	        for (auto& input : Input::inputs) {
	            // release any consumed inputs from rest of game
		        input.update();
	        }
	        result = gui->process();
	    }

        static ConsoleVariable<bool> gui_draw("/gui_draw", true);
        if (*gui_draw) {
		    gui->predraw();
		    gui->draw();
            drawConsoleCommandBuffer();
		    gui->postdraw();
		}
	}
	return result;
}

real_t Player::SkillSheet_t::windowCompactHeightScaleX = 0.0;
real_t Player::SkillSheet_t::windowCompactHeightScaleY = 0.0;
real_t Player::SkillSheet_t::windowHeightScaleX = 0.0;
real_t Player::SkillSheet_t::windowHeightScaleY = 0.0;
bool Player::SkillSheet_t::generateFollowerTableForSkillsheet = false;
SkillSheetFrames_t skillSheetEntryFrames[MAXPLAYERS];

void Player::SkillSheet_t::createSkillSheet()
{
	if ( skillFrame )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player skills %d", player.playernum);
	Frame* frame = gameUIFrame[player.playernum]->addFrame(name);
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
	getColor(fade->color, &r, &g, &b, &a);
	a = 0;
	fade->color = makeColor( r, g, b, a);
	fade->disabled = true;

	Frame* skillBackground = frame->addFrame("skills frame");
	skillBackground->setHollow(true);
	skillSheetEntryFrames[player.playernum].skillsFrame = skillBackground;
	const int width = 684;
	const int height = 404;
	skillBackground->setSize(SDL_Rect{ frame->getSize().x, frame->getSize().y, width, height });
	/*auto bgImg = skillBackground->addImage(SDL_Rect{ 0, 0, width, height }, 0xFFFFFFFF,
		"*#images/ui/SkillSheet/UI_Skills_Window_02.png", "skills img");*/

	Frame* allSkillEntriesLeft = skillBackground->addFrame("skill entries frame left");
	allSkillEntriesLeft->setHollow(true);
	Frame* allSkillEntriesRight = skillBackground->addFrame("skill entries frame right");
	allSkillEntriesRight->setHollow(true);
	skillSheetEntryFrames[player.playernum].entryFrameLeft = allSkillEntriesLeft;
	skillSheetEntryFrames[player.playernum].entryFrameRight = allSkillEntriesRight;
	//allSkillEntries->setSize(SDL_Rect{ 0, 0, skillBackground->getSize().w, skillBackground->getSize().h });

	SDL_Rect allSkillEntriesPosLeft{ 0, 0, 182, skillBackground->getSize().h };
	SDL_Rect allSkillEntriesPosRight{ skillBackground->getSize().w - 182, 0, 182, skillBackground->getSize().h };
	allSkillEntriesLeft->setSize(allSkillEntriesPosLeft);
	allSkillEntriesRight->setSize(allSkillEntriesPosRight);

	allSkillEntriesLeft->addImage(SDL_Rect{ 0, 12, 182, 376 },
		0xFFFFFFFF, "*#images/ui/SkillSheet/UI_Skills_Window_Left_03.png", "bg wing left");
	allSkillEntriesRight->addImage(SDL_Rect{ 0, 12, 182, 376 },
		0xFFFFFFFF, "*#images/ui/SkillSheet/UI_Skills_Window_Right_03.png", "bg wing right");

	auto skillBackgroundImagesFrame = skillBackground->addFrame("skills bg images");
	skillBackgroundImagesFrame->setHollow(true);
	skillSheetEntryFrames[player.playernum].skillBgImgsFrame = skillBackgroundImagesFrame;
	skillBackgroundImagesFrame->setSize(SDL_Rect{ 0, 0, skillBackground->getSize().w, skillBackground->getSize().h });
	{
		Uint32 color = makeColor(255, 255, 255, 255);
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_TL_04.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_TR_04.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_T_04.png", skillsheetEffectBackgroundImages[TOP].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_L_03.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_R_03.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			makeColor(0, 0, 0, 255), "images/system/white.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_BL_03.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_BR_03.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_B_03.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
		imageSetWidthHeight9x9(skillBackgroundImagesFrame, skillsheetEffectBackgroundImages);

		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 78, 18 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_Flourish_T.png", "flourish top");
		skillBackgroundImagesFrame->addImage(SDL_Rect{ 0, 0, 34, 14 },
			color, "*#images/ui/SkillSheet/UI_Skills_Window_Flourish_B.png", "flourish bottom");
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
		entry->setHollow(true);
		skillSheetEntryFrames[player.playernum].entryFrames[i] = entry;
		entry->setSize(skillEntryPos);
		entry->addImage(skillSelectorPos, 0xFFFFFFFF, "*#images/ui/SkillSheet/UI_Skills_SkillSelector_00.png", "selector img");
		SDL_Rect imgBgPos{ skillEntryPos.w - 36, 0, 36, 36 };
		entry->addImage(imgBgPos, 0xFFFFFFFF, "*#images/ui/SkillSheet/UI_Skills_Icons_BG00_00.png", "skill icon bg");
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
		entry->setHollow(true);
		skillSheetEntryFrames[player.playernum].entryFrames[i] = entry;
		entry->setSize(skillEntryPos);
		entry->addImage(skillSelectorPos, 0xFFFFFFFF, "*#images/ui/SkillSheet/UI_Skills_SkillSelectorR_00.png", "selector img");
		SDL_Rect imgBgPos{ 0, 0, 36, 36 };
		entry->addImage(imgBgPos, 0xFFFFFFFF, "*#images/ui/SkillSheet/UI_Skills_Icons_BG00_00.png", "skill icon bg");
		SDL_Rect imgFgPos{ imgBgPos.x + 6, imgBgPos.y + 6, 24, 24 };
		entry->addImage(imgFgPos, 0xFFFFFFFF, "*#images/ui/SkillSheet/icons/Alchemy01.png", "skill icon fg");
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
	skillDescriptionFrame->setHollow(true);
	skillSheetEntryFrames[player.playernum].skillDescFrame = skillDescriptionFrame;
	skillDescriptionFrame->setSize(descPos);

	/*auto debugRect = skillDescriptionFrame->addImage(SDL_Rect{ 0, 0, descPos.w, descPos.h }, 0xFFFFFFFF,
		"images/system/white.png", "")*/;

	auto slider = skillDescriptionFrame->addSlider("skill slider");
	slider->setBorder(24);
	slider->setMinValue(0);
	slider->setMaxValue(100);
	slider->setValue(0);
	SDL_Rect sliderPos{ descPos.w - 34, 4, 30, descPos.h - 8 };
	slider->setRailSize(sliderPos);
	slider->setHandleSize(SDL_Rect{ 0, 0, 34, 34 });
	slider->setOrientation(Slider::SLIDER_VERTICAL);
	//slider->setCallback(callback);
	slider->setColor(makeColor(255, 255, 255, 255));
	slider->setHighlightColor(makeColor(255, 255, 255, 255));
	slider->setHandleImage("*#images/ui/Main Menus/Settings/Settings_Slider_Boulder00.png");
	slider->setRailImage("*#images/ui/Main Menus/Settings/Settings_Slider_Backing00.png");
	slider->setHideGlyphs(true);
	slider->setHideKeyboardGlyphs(true);
	slider->setHideSelectors(true);
	slider->setMenuConfirmControlType(0);

	Font* actualFont = Font::get(descFont);
	int fontHeight;
	actualFont->sizeText("_", nullptr, &fontHeight);

	auto scrollAreaOuterFrame = skillDescriptionFrame->addFrame("scroll area outer frame");
	scrollAreaOuterFrame->setHollow(true);
	skillSheetEntryFrames[player.playernum].scrollAreaOuterFrame = scrollAreaOuterFrame;
	scrollAreaOuterFrame->setSize(SDL_Rect{ 16, 4, sliderPos.x - 4 - 16, descPos.h - 8 });
	auto scrollAreaFrame = scrollAreaOuterFrame->addFrame("skill scroll area");
	scrollAreaFrame->setHollow(true);
	skillSheetEntryFrames[player.playernum].scrollArea = scrollAreaFrame;
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
			0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_DEX_00.png",	"stat icon");

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
		skillDescriptionBgFrame->setHollow(true);
		{
			//Uint32 color = makeColor(22, 24, 29, 255);
			Uint32 color = makeColor(255, 255, 255, 128);
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_TL_00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_TR_00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_T_00.png", skillsheetEffectBackgroundImages[TOP].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_ML_00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_MR_00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_M_00.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_BL_00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_BR_00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
			skillDescriptionBgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/SkillSheet/UI_Skills_LegendBox_B_00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());
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
			effectFrame->setHollow(true);
			skillSheetEntryFrames[player.playernum].effectFrames[i] = effectFrame;
			effectFrame->setSize(SDL_Rect{ txtPos.x, txtPos.y - 2, txtPos.w, fontHeight + 8 });
			effectFrame->addImage(
				SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h - 4 }, 
				makeColor(101, 87, 67, 255), "images/system/white.png", "effect frame bg highlight");
			effectFrame->addImage(
				SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h - 4 },
				makeColor(101, 33, 33, 28), "images/system/white.png", "effect frame bg tmp");
			int valueX =  effectFrame->getSize().w - effectXOffset;

			auto valBgImgFrame = effectFrame->addFrame("effect val bg frame");
			valBgImgFrame->setHollow(true);
			valBgImgFrame->setSize(SDL_Rect{ valueX - effectBackgroundXOffset, 0, effectBackgroundWidth, effectFrame->getSize().h - 4});
			{
				Uint32 color = makeColor(51, 33, 26, 255);
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_TL00.png", skillsheetEffectBackgroundImages[TOP_LEFT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_TR00.png", skillsheetEffectBackgroundImages[TOP_RIGHT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_T00.png", skillsheetEffectBackgroundImages[TOP].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_L00.png", skillsheetEffectBackgroundImages[MIDDLE_LEFT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_R00.png", skillsheetEffectBackgroundImages[MIDDLE_RIGHT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_M00.png", skillsheetEffectBackgroundImages[MIDDLE].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_BL00.png", skillsheetEffectBackgroundImages[BOTTOM_LEFT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_BR00.png", skillsheetEffectBackgroundImages[BOTTOM_RIGHT].c_str());
				valBgImgFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_B00.png", skillsheetEffectBackgroundImages[BOTTOM].c_str());

				/*valBgImgFrame->addImage(
					SDL_Rect{ 0, 0, effectFrame->getSize().w, effectFrame->getSize().h - 4 },
					makeColor(255, 255, 255, 128), "images/system/white.png", "tmp tmp");*/
				imageSetWidthHeight9x9(valBgImgFrame, skillsheetEffectBackgroundImages);
			}

			auto effectTxtFrame = effectFrame->addFrame("effect txt frame");
			effectTxtFrame->setHollow(true);
			effectTxtFrame->setSize(SDL_Rect{ 0, 0, effectFrame->getSize().w - effectXOffset - effectBackgroundXOffset, effectFrame->getSize().h - 4 });
			auto effectTxt = effectTxtFrame->addField("effect txt", 1024);
			effectTxt->setFont(descFont);
			effectTxt->setSize(SDL_Rect{0, 0, 1000, effectTxtFrame->getSize().h}); // large 1000px to handle large text length marquee
			effectTxt->setVJustify(Field::justify_t::CENTER);
			effectTxt->setText("");
			effectTxt->setColor(makeColor(201, 162, 100, 255));

			auto effectValFrame = effectFrame->addFrame("effect val frame");
			effectValFrame->setHollow(true);
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
			"*#images/ui/SkillSheet/UI_Skills_Separator_00.png", "legend div");
		legendDivImg->pos.x = scrollAreaFrame->getSize().w / 2 - legendDivImg->pos.w / 2;
		auto legendDivTxt = scrollAreaFrame->addField("legend div text", 128);
		legendDivTxt->setText(language[4056]);
		SDL_Rect legendDivTxtPos = legendDivImg->pos;
		legendDivTxt->setSize(legendDivTxtPos);
		legendDivTxt->setFont(descFont);
		legendDivTxt->setHJustify(Field::justify_t::CENTER);

		auto legendFrame = scrollAreaFrame->addFrame("legend frame");
		legendFrame->setHollow(true);
		skillSheetEntryFrames[player.playernum].legendFrame = legendFrame;
		SDL_Rect legendPos{ 0, txtPos.y, txtPos.w, 100 };
		legendFrame->setSize(legendPos);
		auto tl = legendFrame->addImage(SDL_Rect{ 0, 0, 18, 18 }, 0xFFFFFFFF, 
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_TL_00.png", "top left img");
		auto tm = legendFrame->addImage(SDL_Rect{ 18, 0, legendPos.w - 18 * 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_T_00.png", "top img");
		auto tr = legendFrame->addImage(SDL_Rect{ legendPos.w - 18, 0, 18, 18 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_TR_00.png", "top right img");

		auto ml = legendFrame->addImage(SDL_Rect{ 0, 18, 18, legendPos.h - 18 * 2 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_ML_00.png", "middle left img");
		auto mm = legendFrame->addImage(SDL_Rect{ 18, 18, legendPos.w - 18 * 2, ml->pos.h }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_M_00.png", "middle img");
		auto mr = legendFrame->addImage(SDL_Rect{ legendPos.w - 18, 18, 18, ml->pos.h }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_MR_00.png", "middle right img");

		auto bl = legendFrame->addImage(SDL_Rect{ 0, ml->pos.y + ml->pos.h, 18, 18 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_BL_00.png", "bottom left img");
		auto bm = legendFrame->addImage(SDL_Rect{ 18, bl->pos.y, legendPos.w - 18 * 2, 18 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_B_00.png", "bottom img");
		auto br = legendFrame->addImage(SDL_Rect{ legendPos.w - 18, bl->pos.y, 18, 18 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendBox_BR_00.png", "bottom right img");

		auto backerImg = scrollAreaFrame->addImage(SDL_Rect{ 0, 0, 154, 12 }, 0xFFFFFFFF,
			"*#images/ui/SkillSheet/UI_Skills_LegendTextBacker_00.png", "legend txt backer img");
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
			skillEffect.marqueeTicks[player.playernum] = 0;
			skillEffect.marquee[player.playernum] = 0.0;
			skillEffect.marqueeCompleted[player.playernum] = false;
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
	else
	{
		player.GUI.returnToPreviousActiveModule();
	}
}

void Player::SkillSheet_t::openSkillSheet()
{
	player.GUI.previousModule = player.GUI.activeModule;
	if ( player.shootmode )
	{
		players[player.playernum]->openStatusScreen(GUI_MODE_NONE,
			INVENTORY_MODE_ITEM, player.GUI.MODULE_SKILLS_LIST);
	}
	else
	{
		players[player.playernum]->openStatusScreen(GUI_MODE_INVENTORY,
			players[player.playernum]->inventory_mode, player.GUI.MODULE_SKILLS_LIST); // Reset the GUI to the inventory.
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
	if ( bUseCompactSkillsView || bSlideWindowsOnly )
	{
		if ( selectedSkill >= 8 )
		{
			skillSlideAmount = 1.0;
			skillSlideDirection = 1;
		}
		else
		{
			skillSlideAmount = -1.0;
			skillSlideDirection = -1;
		}
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
			val = stats[playernum]->getActiveShieldBonus(false);
			snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
		}
		else if ( tag == "PASSIVE_AC_INCREASE" )
		{
			val = stats[playernum]->getPassiveShieldBonus(false);
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
			val /= 1.f + statGetCHR(stats[playernum], players[playernum]->entity) / 20.f;
			val = std::max(1.0, val);
			val = val * 100.0 - 100.0;
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "TRADING_SELL_PRICE" )
		{
			val = (50 + stats[playernum]->PROFICIENCIES[proficiency]) / 150.f; // sell value
			val *= 1.f + statGetCHR(stats[playernum], players[playernum]->entity) / 20.f;
			val = std::min(1.0, val);
			val = val * 100.0 - 100.0;
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
				val = getBaseManaRegen(player, *(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
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
				int skill = stats[playernum]->PROFICIENCIES[proficiency];
				int multiplier = (skill / 20) + 1;
				val = multiplier;
				//real_t normalValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				//stats[playernum]->PROFICIENCIES[proficiency] = 0;
				//real_t zeroValue = player->getBaseManaRegen(*(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
				//stats[playernum]->PROFICIENCIES[proficiency] = skill;
				//
				//val = (100 * zeroValue / normalValue) - 100;
				snprintf(buf, sizeof(buf), rawValue.c_str(), (int)val);
			}
		}
		else if ( tag == "CASTING_MP_REGEN_SKILL_BONUS" )
		{
			val = 0.0;
			int skill = stats[playernum]->PROFICIENCIES[proficiency];
			real_t normalValue = getBaseManaRegen(player, *(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
			stats[playernum]->PROFICIENCIES[proficiency] = 0;
			real_t zeroValue = getBaseManaRegen(player, *(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
			stats[playernum]->PROFICIENCIES[proficiency] = skill;
				
			val = (100 * zeroValue / normalValue) - 100;
			snprintf(buf, sizeof(buf), rawValue.c_str(), val);
		}
		else if ( tag == "CASTING_MP_REGEN_BONUS_INT" )
		{
			val = 0.0;
			int stat = stats[playernum]->INT;
			real_t normalValue = getBaseManaRegen(player, *(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
			stats[playernum]->INT = 0;
			real_t zeroValue = getBaseManaRegen(player, *(stats[playernum])) / (TICKS_PER_SECOND * 1.f);
			stats[playernum]->INT = stat;

			val = (100 * zeroValue / normalValue) - 100;
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

	if ( !player.usingCommand() && Input::inputs[player.playernum].consumeBinaryToggle("Skill Sheet")
		&& !gamePaused && players[player.playernum]->bControlEnabled )
	{
		if ( !bSkillSheetOpen )
		{
			openSkillSheet();
		}
		else
		{
			closeSkillSheet();
		}
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
		getColor(fade->color, &r, &g, &b, &a);
		a = 0;
		fade->color = makeColor( r, g, b, a);
		fade->disabled = true;
		return;
	}

	skillFrame->setDisabled(false);
	skillFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		players[player.playernum]->camera_virtualWidth(),
		players[player.playernum]->camera_virtualHeight() });

	bool oldCompactViewVal = bUseCompactSkillsView;
	bool oldSlideWindowsOnly = bSlideWindowsOnly;
	if ( splitscreen && (player.bUseCompactGUIHeight() || player.bUseCompactGUIWidth()) )
	{
		// use compact view.
		if ( player.bUseCompactGUIWidth() && !player.bUseCompactGUIHeight() )
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
		if ( selectedSkill >= 8 )
		{
			skillSlideAmount = 1.0;
			skillSlideDirection = 1;
		}
		else
		{
			skillSlideAmount = -1.0;
			skillSlideDirection = -1;
		}
	}
	if ( !bUseCompactSkillsView && !bSlideWindowsOnly )
	{
		skillSlideAmount = 0.0;
		skillSlideDirection = 0;
	}

	auto innerFrame = skillSheetEntryFrames[player.playernum].skillsFrame;
	SDL_Rect sheetSize = innerFrame->getSize();
	Frame* allSkillEntriesLeft = skillSheetEntryFrames[player.playernum].entryFrameLeft;
	Frame* allSkillEntriesRight = skillSheetEntryFrames[player.playernum].entryFrameRight;
	auto leftWingImg = allSkillEntriesLeft->findImage("bg wing left");
	auto rightWingImg = allSkillEntriesRight->findImage("bg wing right");
	auto skillDescriptionFrame = skillSheetEntryFrames[player.playernum].skillDescFrame;
	auto bgImgFrame = skillSheetEntryFrames[player.playernum].skillBgImgsFrame;
	auto scrollAreaOuterFrame = skillSheetEntryFrames[player.playernum].scrollAreaOuterFrame;
	auto scrollArea = skillSheetEntryFrames[player.playernum].scrollArea;
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
			leftWingImg->path = "*#images/ui/SkillSheet/UI_Skills_Window_LeftCompact_03.png";
			rightWingImg->path = "*#images/ui/SkillSheet/UI_Skills_Window_RightCompact_03.png";
		}
		else
		{
			leftWingImg->path = "*#images/ui/SkillSheet/UI_Skills_Window_Left_03.png";
			rightWingImg->path = "*#images/ui/SkillSheet/UI_Skills_Window_Right_03.png";
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
	getColor(fade->color, &r, &g, &b, &a);
	a = 128 * skillsFadeInAnimationY;
	fade->color = makeColor( r, g, b, a);
	fade->disabled = false;

	int baseY = (skillFrame->getSize().h / 2 - sheetSize.h / 2);
	sheetSize.y = -sheetSize.h + skillsFadeInAnimationY * (baseY + sheetSize.h);
	innerFrame->setSize(sheetSize);

	auto slider = skillDescriptionFrame->findSlider("skill slider");
	if ( slider->isSelected() )
	{
		slider->deselect();
	}
	bool sliderDisabled = slider->isDisabled();

	auto titleText = innerFrame->findField("skill title txt");
	SDL_Rect titleTextPos = titleText->getSize();
	titleTextPos.x = skillDescriptionFrame->getSize().x + skillDescriptionFrame->getSize().w / 2 - titleTextPos.w / 2;
	titleText->setSize(titleTextPos);

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
		if ( Input::inputs[player.playernum].binaryToggle("MenuUp")
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
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
		if ( Input::inputs[player.playernum].binaryToggle("MenuDown")
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
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
		if ( Input::inputs[player.playernum].binaryToggle("MenuLeft") 
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
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
		if ( Input::inputs[player.playernum].binaryToggle("MenuRight")
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
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
			inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
		}
		if ( Input::inputs[player.playernum].binaryToggle("MenuCancel")
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
		{
			Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
			closeSheetAction = true;
		}
	}
	bool mouseClickedOutOfBounds = false;
	if ( inputs.bPlayerUsingKeyboardControl(player.playernum) && Input::inputs[player.playernum].binaryToggle("MenuLeftClick") )
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
			auto entry = skillSheetEntryFrames[player.playernum].entryFrames[i];

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
					if ( Input::inputs[player.playernum].binaryToggle("MenuLeftClick") )
					{
						selectSkill(i);
						Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
					}
				}
			}

			if ( selectedSkill == i || highlightedSkill == i )
			{
				selectorIcon->disabled = false;
				if ( selectedSkill == i && highlightedSkill == i )
				{
					selectorIcon->path = (i < 8) ? "*#images/ui/SkillSheet/UI_Skills_SkillSelector100_01.png" : "*#images/ui/SkillSheet/UI_Skills_SkillSelector100R_01.png";
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
				auto effectFrame = skillSheetEntryFrames[player.playernum].effectFrames[eff];
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
							effect_t.marquee[player.playernum] += (.005 * fpsScale);
							//effect_t.marquee[player.playernum] = std::min(1.0, effect_t.marquee[player.playernum]);

							/*if ( effect_t.marqueeTicks[player.playernum] == 0 && effect_t.marquee[player.playernum] >= 1.0 )
							{
								effect_t.marqueeTicks[player.playernum] = ticks;
							}*/
							/*if ( effect_t.marqueeTicks[player.playernum] > 0 && (ticks - effect_t.marqueeTicks[player.playernum] > TICKS_PER_SECOND * 2) )
							{
								effect_t.marqueeTicks[player.playernum] = 0;
								effect_t.marquee[player.playernum] = 0.0;
							}*/
						}
					}
					SDL_Rect posTitle = effectTxt->getSize();
					int scrollTitleLength = titleWidth - effectTxtFrame->getSize().w;
					if ( titleWidth <= effectTxtFrame->getSize().w )
					{
						scrollTitleLength = 0;
						posTitle.x = 0;
						effect_t.marqueeCompleted[player.playernum] = false;
						effect_t.marquee[player.playernum] = 0.0;
						effect_t.marqueeTicks[player.playernum] = 0;
					}
					else
					{
						posTitle.x = std::max((int)(-effect_t.marquee[player.playernum] * 100), -scrollTitleLength);
						if ( posTitle.x == -scrollTitleLength )
						{
							if ( !effect_t.marqueeCompleted[player.playernum] )
							{
								effect_t.marqueeTicks[player.playernum] = ticks;
							}
							effect_t.marqueeCompleted[player.playernum] = true;
						}
						else
						{
							effect_t.marqueeCompleted[player.playernum] = false;
						}
					}
					//posTitle.x = -scrollTitleLength * effect_t.marquee[player.playernum];
					effectTxt->setSize(posTitle);

					SDL_Rect posValue = effectVal->getSize();
					int scrollValueLength = valueWidth - effectValFrame->getSize().w;
					if ( valueWidth <= effectValFrame->getSize().w )
					{
						scrollValueLength = 0;
					}
					posValue.x = -scrollValueLength * effect_t.marquee[player.playernum];
					effectVal->setSize(posValue);
				}

				previousEffectFrameHeight = effectFrame->getSize().y + effectFrame->getSize().h;
			}

			Uint32 lastMarqueeTick = 0;
			bool allMarqueeCompleted = true;
			for ( auto& effect_t : skillSheetData.skillEntries[selectedSkill].effects )
			{
				if ( effect_t.marquee[player.playernum] > 0.0 )
				{
					if ( !effect_t.marqueeCompleted[player.playernum] )
					{
						allMarqueeCompleted = false;
					}
					lastMarqueeTick = std::max(effect_t.marqueeTicks[player.playernum], lastMarqueeTick);
				}
			}
			if ( allMarqueeCompleted && lastMarqueeTick > 0 && ((ticks - lastMarqueeTick) > 2 * TICKS_PER_SECOND) )
			{
				for ( auto& effect_t : skillSheetData.skillEntries[selectedSkill].effects )
				{
					effect_t.marquee[player.playernum] = 0.0;
					effect_t.marqueeCompleted[player.playernum] = false;
					effect_t.marqueeTicks[player.playernum] = 0;
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

			auto legendFrame = skillSheetEntryFrames[player.playernum].legendFrame;
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
			if ( skillSheetEntryFrames[player.playernum].legendTextRequiresReflow )
			{
				legendText->reflowTextToFit(0);
			}
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
				tl->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_TL_00.png";
				tm->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_T_00.png";
				tr->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_TR_00.png";

				ml->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_ML_00.png";
				mm->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_M_00.png";
				mr->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_MR_00.png";

				bl->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_BL_00.png";
				bm->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_B_00.png";
				br->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox_BR_00.png";
			}
			else
			{
				legendDivTxt->setColor(legendGoldColor);
				legendText->setColor(legendGoldColor);
				tl->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_TL_00.png";
				tm->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_T_00.png";
				tr->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_TR_00.png";

				ml->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_ML_00.png";
				mm->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_M_00.png";
				mr->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_MR_00.png";

				bl->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_BL_00.png";
				bm->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_B_00.png";
				br->path = "*#images/ui/SkillSheet/UI_Skills_LegendBox100_BR_00.png";
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

		if ( !slider->isDisabled() && player.bControlEnabled && !gamePaused && !player.usingCommand() )
		{
			if ( inputs.bPlayerUsingKeyboardControl(player.playernum) )
			{
				if ( Input::inputs[player.playernum].binaryToggle("MenuMouseWheelDownAlt") )
				{
					Input::inputs[player.playernum].consumeBinaryToggle("MenuMouseWheelDownAlt");
					scrollInertia = std::min(scrollInertia + .05, .15);
				}
				if ( Input::inputs[player.playernum].binaryToggle("MenuMouseWheelUpAlt") )
				{
					Input::inputs[player.playernum].consumeBinaryToggle("MenuMouseWheelUpAlt");
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
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
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
		promptImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuScrollDown");
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
			Input::inputs[player.playernum].consumeBinaryToggle("MenuLeftClick");
			Input::inputs[player.playernum].consumeBindingsSharedWithBinding("MenuLeftClick");
			if ( inputs.bPlayerUsingKeyboardControl(player.playernum) )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
			}
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

void Player::Inventory_t::SpellPanel_t::openSpellPanel()
{
	if ( player.inventoryUI.spellFrame )
	{
		bool wasDisabled = player.inventoryUI.spellFrame->isDisabled();
		player.inventoryUI.spellFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			isInteractable = false;
			currentScrollRow = 0;
			scrollPercent = 0.0;
			scrollInertia = 0.0;
			bFirstTimeSnapCursor = false;
		}
		bOpen = true;
	}
}

void Player::Inventory_t::SpellPanel_t::closeSpellPanel()
{
	if ( player.inventoryUI.spellFrame )
	{
		player.inventoryUI.spellFrame->setDisabled(true);
	}
	animx = 0.0;
	isInteractable = false;
	currentScrollRow = 0;
	scrollPercent = 0.0;
	scrollInertia = 0.0;
	scrollAnimateX = scrollSetpoint;
	bOpen = false;
	bFirstTimeSnapCursor = false;
}

int Player::Inventory_t::SpellPanel_t::getNumSpellsToDisplayVertical() const
{
	if ( !player.bUseCompactGUIHeight() )
	{
		return kNumSpellsToDisplayVertical;
	}
	else
	{
		return kNumSpellsToDisplayVertical - 2;
	}
}

int Player::Inventory_t::SpellPanel_t::heightOffsetWhenNotCompact = 50;
void Player::Inventory_t::SpellPanel_t::updateSpellPanel()
{
	Frame* spellFrame = player.inventoryUI.spellFrame;
	if ( !spellFrame ) { return; }

	if ( !spellFrame->isDisabled() && bOpen )
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				if ( !inputs.getUIInteraction(player.playernum)->selectedItem
					&& player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
				{
					player.inventoryUI.warpMouseToSelectedSpell(nullptr, (Inputs::SET_CONTROLLER));
				}
			}
			isInteractable = true;
		}
	}
	else
	{
		animx = 0.0;
		isInteractable = false;
		scrollInertia = 0.0;
	}
	auto spellFramePos = spellFrame->getSize();
	if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		spellFramePos.x = -spellFramePos.w + animx * spellFramePos.w;
		if ( player.bUseCompactGUIWidth() )
		{
			if ( player.inventoryUI.slideOutPercent >= .0001 )
			{
				isInteractable = false;
			}
			spellFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
		}
	}
	else
	{
		spellFramePos.x = player.camera_virtualWidth() - animx * spellFramePos.w;
		if ( player.bUseCompactGUIWidth() )
		{
			if ( player.inventoryUI.slideOutPercent >= .0001 )
			{
				isInteractable = false;
			}
			spellFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
		}
	}
	spellFrame->setSize(spellFramePos);

	auto baseFrame = spellFrame->findFrame("spell base");
	auto slider = baseFrame->findSlider("spell slider");
	auto spellSlotsFrame = spellFrame->findFrame("spell slots");
	auto baseBackgroundImg = baseFrame->findImage("spell base img");
	auto baseHeaderImg = baseFrame->findImage("spell header img");
	// handle height changing..
	{
		int frameHeight = kSpellListHeight;
		int totalFrameHeightChange = 0;
		if ( !player.bUseCompactGUIHeight() )
		{
			totalFrameHeightChange = heightOffsetWhenNotCompact;
		}
		spellFramePos.h = frameHeight + totalFrameHeightChange;
		spellFrame->setSize(spellFramePos);
		baseBackgroundImg->pos.y = totalFrameHeightChange;
		baseHeaderImg->pos.y = baseBackgroundImg->pos.y - baseHeaderImg->pos.h + 8;
		SDL_Rect spellBasePos = baseFrame->getSize();
		spellBasePos.h = spellFramePos.h;
		baseFrame->setSize(spellBasePos);

		int numGrids = (players[player.playernum]->inventoryUI.MAX_SPELLS_Y / getNumSpellsToDisplayVertical()) + 1;
		SDL_Rect spellSlotsFramePos = spellSlotsFrame->getSize();

		int heightChange = 0;
		if ( getNumSpellsToDisplayVertical() < kNumSpellsToDisplayVertical )
		{
			heightChange = player.inventoryUI.getSlotSize() * (kNumSpellsToDisplayVertical - getNumSpellsToDisplayVertical());
		}
		spellSlotsFramePos.y = 4 + heightChange + totalFrameHeightChange;
		spellSlotsFramePos.h = 242 - heightChange;
		spellSlotsFrame->setActualSize(SDL_Rect{ spellSlotsFrame->getActualSize().x, 
			spellSlotsFrame->getActualSize().y, 
			spellSlotsFrame->getActualSize().w, 
			(spellSlotsFramePos.h) * numGrids });
		spellSlotsFrame->setSize(spellSlotsFramePos);
		auto gridImg = spellSlotsFrame->findImage("grid img");
		gridImg->pos.y = kSpellListGridY;
		gridImg->pos.h = (spellSlotsFramePos.h) * numGrids;

		SDL_Rect sliderPos = slider->getRailSize();
		sliderPos.y = 8 + heightChange + totalFrameHeightChange;
		sliderPos.h = 234 - heightChange;
		slider->setRailSize(sliderPos);
	}


	int lowestItemY = getNumSpellsToDisplayVertical() - 1;
	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }
		if ( itemCategory(item) != SPELL_CAT ) { continue; }

		lowestItemY = std::max(lowestItemY, item->y);
	}

	int scrollAmount = std::max((lowestItemY + 1) - (getNumSpellsToDisplayVertical()), 0) * player.inventoryUI.getSlotSize();
	if ( scrollAmount == 0 )
	{
		slider->setDisabled(true);
	}
	else
	{
		slider->setDisabled(false);
	}

	currentScrollRow = scrollSetpoint / player.inventoryUI.getSlotSize();

	if ( bOpen && isInteractable )
	{
		// do sliders
		if ( !slider->isDisabled() )
		{
			if ( !inputs.getUIInteraction(player.playernum)->selectedItem
				&& player.GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
			{
				if ( inputs.bPlayerUsingKeyboardControl(player.playernum) )
				{
					if ( Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] )
					{
						Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] = 0;
						scrollSetpoint = std::max(scrollSetpoint + player.inventoryUI.getSlotSize(), 0);
					}
					if ( Input::mouseButtons[Input::MOUSE_WHEEL_UP] )
					{
						Input::mouseButtons[Input::MOUSE_WHEEL_UP] = 0;
						scrollSetpoint = std::max(scrollSetpoint - player.inventoryUI.getSlotSize(), 0);
					}
				}

				if ( Input::inputs[player.playernum].analogToggle("MenuScrollDown") )
				{
					Input::inputs[player.playernum].consumeAnalogToggle("MenuScrollDown");
					scrollSetpoint = std::max(scrollSetpoint + player.inventoryUI.getSlotSize(), 0);
					if ( player.inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_SPELLS )
					{
						player.inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
					}
				}
				else if ( Input::inputs[player.playernum].analogToggle("MenuScrollUp") )
				{
					Input::inputs[player.playernum].consumeAnalogToggle("MenuScrollUp");
					scrollSetpoint = std::max(scrollSetpoint - player.inventoryUI.getSlotSize(), 0);
					if ( player.inventoryUI.cursor.queuedModule == Player::GUI_t::MODULE_SPELLS )
					{
						player.inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
					}
				}
			}
		}

		scrollSetpoint = std::min(scrollSetpoint, scrollAmount);
		currentScrollRow = scrollSetpoint / player.inventoryUI.getSlotSize();

		if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
		{
			isInteractable = false;
			player.inventoryUI.tooltipDelayTick = ticks + TICKS_PER_SECOND / 10;
			const real_t fpsScale = (60.f / std::max(1U, fpsLimit));
			real_t setpointDiff = 0.0;
			if ( scrollSetpoint - scrollAnimateX > 0.0 )
			{
				setpointDiff = fpsScale * std::max(3.0, (scrollSetpoint - scrollAnimateX)) / 3.0;
			}
			else
			{
				setpointDiff = fpsScale * std::min(-3.0, (scrollSetpoint - scrollAnimateX)) / 3.0;
			}
			scrollAnimateX += setpointDiff;
			if ( setpointDiff > 0.0 )
			{
				scrollAnimateX = std::min((real_t)scrollSetpoint, scrollAnimateX);
			}
			else
			{
				scrollAnimateX = std::max((real_t)scrollSetpoint, scrollAnimateX);
			}
		}
		else
		{
			scrollAnimateX = scrollSetpoint;
		}

		if ( !inputs.getUIInteraction(player.playernum)->selectedItem 
			&& !player.GUI.isDropdownActive()
			&& player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_SPELLS)
			&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
		{
			if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
				player.GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
				player.inventoryUI.cycleInventoryTab();
				player.inventoryUI.spellPanel.closeSpellPanel();
			}
		}
	}

	if ( scrollAmount > 0 )
	{
		slider->setValue((scrollAnimateX / scrollAmount) * 100.0);
	}
	else
	{
		slider->setValue(0.0);
	}

	SDL_Rect actualSize = spellSlotsFrame->getActualSize();
	actualSize.y = scrollAnimateX;
	spellSlotsFrame->setActualSize(actualSize);
}

bool Player::Inventory_t::SpellPanel_t::isSlotVisible(int x, int y) const
{
	if ( player.inventoryUI.spellFrame )
	{
		if ( player.inventoryUI.spellFrame->isDisabled() )
		{
			return false;
		}
	}
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumSpellsToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		return true;
	}
	return false;
}

bool Player::Inventory_t::SpellPanel_t::isItemVisible(Item* item) const
{
	if ( !item ) { return false; }
	return isSlotVisible(item->x, item->y);
}

void Player::Inventory_t::SpellPanel_t::scrollToSlot(int x, int y, bool instantly)
{
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumSpellsToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		// no work to do.
		return;
	}

	int lowestItemY = getNumSpellsToDisplayVertical() - 1;
	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }
		if ( itemCategory(item) != SPELL_CAT ) { continue; }

		lowestItemY = std::max(lowestItemY, item->y);
	}
	int maxScroll = std::max((lowestItemY + 1) - (getNumSpellsToDisplayVertical()), 0) * player.inventoryUI.getSlotSize();
	
	int scrollAmount = 0;
	if ( y < lowerY )
	{
		scrollAmount = (y) * player.inventoryUI.getSlotSize();
		//scrollAmount += scrollSetpoint;
	}
	else if ( y > upperY )
	{
		scrollAmount = (y - upperY) * player.inventoryUI.getSlotSize();
		scrollAmount += scrollSetpoint;
	}
	scrollAmount = std::min(scrollAmount, maxScroll);

	scrollSetpoint = scrollAmount;
	if ( instantly )
	{
		scrollAnimateX = scrollSetpoint;
	}
	currentScrollRow = scrollSetpoint / player.inventoryUI.getSlotSize();
	if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
	{
		isInteractable = false;
	}
}

void Player::Inventory_t::ChestGUI_t::openChest()
{
	if ( player.inventoryUI.chestFrame )
	{
		bool wasDisabled = player.inventoryUI.chestFrame->isDisabled();
		player.inventoryUI.chestFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			isInteractable = false;
			currentScrollRow = 0;
			scrollPercent = 0.0;
			scrollInertia = 0.0;
			bFirstTimeSnapCursor = false;
		}
		/*if ( player.inventoryUI.getSelectedChestX() < 0 || player.inventoryUI.getSelectedChestX() >= MAX_CHEST_X
			|| player.inventoryUI.getSelectedChestY() < 0 || player.inventoryUI.getSelectedChestY() >= MAX_CHEST_Y )*/
		{
			player.inventoryUI.selectChestSlot(0, 0); // always select first slot
		}
		player.hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player.inventory_mode = INVENTORY_MODE_ITEM;
		bOpen = true;
	}
	if ( inputs.getUIInteraction(player.playernum)->selectedItem )
	{
		inputs.getUIInteraction(player.playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(player.playernum)->toggleclick = false;
	}
	inputs.getUIInteraction(player.playernum)->selectedItemFromChest = 0;
}

bool Player::Inventory_t::ChestGUI_t::isChestSelected()
{
	if ( !bOpen )
	{
		return false;
	}

	if ( player.GUI.activeModule == Player::GUI_t::MODULE_CHEST )
	{
		if ( selectedChestSlotX >= 0 && selectedChestSlotX < MAX_CHEST_X
			&& selectedChestSlotY >= 0 && selectedChestSlotY < MAX_CHEST_Y )
		{
			return true;
		}
	}

	return false;
}

void Player::Inventory_t::ChestGUI_t::closeChest()
{
	if ( player.inventoryUI.chestFrame )
	{
		player.inventoryUI.chestFrame->setDisabled(true);
	}
	animx = 0.0;
	isInteractable = false;
	currentScrollRow = 0;
	scrollPercent = 0.0;
	scrollInertia = 0.0;
	scrollAnimateX = scrollSetpoint;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	if ( inputs.getUIInteraction(player.playernum)->selectedItemFromChest > 0 )
	{
		inputs.getUIInteraction(player.playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(player.playernum)->toggleclick = false;
	}
	inputs.getUIInteraction(player.playernum)->selectedItemFromChest = 0;
	if ( player.GUI.dropdownMenu.currentName == "chest_interact" )
	{
		player.GUI.dropdownMenu.close();
	}
	if ( players[player.playernum]->GUI.activeModule == Player::GUI_t::MODULE_CHEST
		&& !players[player.playernum]->shootmode )
	{
		// reset to inventory mode if still hanging in chest GUI
		players[player.playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[player.playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			players[player.playernum]->GUI.warpControllerToModule(false);
		}
	}
}

int Player::Inventory_t::ChestGUI_t::getNumItemsToDisplayVertical() const
{
	return kNumItemsToDisplayVertical;
}

void Player::Inventory_t::selectChestSlot(const int x, const int y)
{
	chestGUI.selectedChestSlotX = x;
	chestGUI.selectedChestSlotY = y;
}

const bool Player::Inventory_t::isItemFromChest(Item* item) const
{
	if ( !item )
	{
		return false;
	}

	if ( !openedChest[player.playernum] || !chestGUI.bOpen )
	{
		return false;
	}

	list_t* chest_inventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv[player.playernum];
	}
	else if ( openedChest[player.playernum]->children.first && openedChest[player.playernum]->children.first->element )
	{
		chest_inventory = (list_t*)openedChest[player.playernum]->children.first->element;
	}

	if ( item->node && item->node->list == chest_inventory )
	{
		return true;
	}
	return false;
}

int Player::Inventory_t::ChestGUI_t::heightOffsetWhenNotCompact = 0;
void Player::Inventory_t::ChestGUI_t::updateChest()
{
	updateChestInventory(player.playernum);

	Frame* chestFrame = player.inventoryUI.chestFrame;
	if ( !chestFrame ) { return; }

	auto baseFrame = chestFrame->findFrame("chest base");
	auto grabAllBtn = baseFrame->findButton("grab all button");
	auto closeBtn = baseFrame->findButton("close chest button");
	if ( !chestFrame->isDisabled() && bOpen )
	{
		const real_t fpsScale = (50.f / std::max(1U, fpsLimit)); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				if ( !inputs.getUIInteraction(player.playernum)->selectedItem
					&& player.GUI.activeModule == Player::GUI_t::MODULE_CHEST )
				{
					player.inventoryUI.warpMouseToSelectedChestSlot(nullptr, (Inputs::SET_CONTROLLER));
				}
			}
			isInteractable = true;
		}
	}
	else
	{
		animx = 0.0;
		isInteractable = false;
		scrollInertia = 0.0;
	}

	auto chestFramePos = chestFrame->getSize();
	auto baseBackgroundImg = baseFrame->findImage("chest base img");
	auto lidBackgroundImg = baseFrame->findImage("chest lid img");

	if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player.inventoryUI.bCompactView )
		{
			const int fullWidth = chestFramePos.w + 210; // inventory width 210
			chestFramePos.x = -chestFramePos.w + animx * fullWidth;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				chestFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
		else
		{
			chestFramePos.x = player.camera_virtualWidth() - animx * chestFramePos.w;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				chestFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
	}
	else if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
	{
		if ( !player.inventoryUI.bCompactView )
		{
			chestFramePos.x = player.camera_virtualWidth() - animx * chestFramePos.w * 2;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				chestFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
		else
		{
			chestFramePos.x = -chestFramePos.w + animx * chestFramePos.w;
			if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				chestFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}
		}
	}
	chestFrame->setSize(chestFramePos);

	bool drawGlyphs = !::inputs.getVirtualMouse(player.playernum)->draw_cursor;
	if ( !drawGlyphs )
	{
		closeBtn->setInvisible(false);
		grabAllBtn->setInvisible(false);
		grabAllBtn->setDisabled(!isInteractable);
		closeBtn->setDisabled(!isInteractable);
	}
	else
	{
		closeBtn->setInvisible(true);
		grabAllBtn->setInvisible(true);
		grabAllBtn->setDisabled(true);
		closeBtn->setDisabled(true);
	}

	//auto slider = baseFrame->findSlider("chest slider");
	auto chestSlotsFrame = chestFrame->findFrame("chest slots");

	auto promptBack = baseFrame->findField("prompt back txt");
	promptBack->setDisabled(!drawGlyphs);
	auto promptBackImg = baseFrame->findImage("prompt back img");
	promptBackImg->disabled = !drawGlyphs;
	auto promptGrab = baseFrame->findField("prompt grab txt");
	promptGrab->setDisabled(!drawGlyphs);
	auto promptGrabImg = baseFrame->findImage("prompt grab img");
	promptGrabImg->disabled = !drawGlyphs;
	// handle height changing..
	{
		int frameHeight = 236 + 16;
		if ( !player.bUseCompactGUIHeight() )
		{
			chestFramePos.y = 130;
		}
		else
		{
			chestFramePos.y = 0;
		}
		chestFramePos.h = frameHeight;
		chestFrame->setSize(chestFramePos);
		baseBackgroundImg->pos.y = 16 + 48;
		lidBackgroundImg->pos.y = baseBackgroundImg->pos.y - lidBackgroundImg->pos.h + 2;
		SDL_Rect chestBasePos = baseFrame->getSize();
		chestBasePos.h = chestFramePos.h;
		baseFrame->setSize(chestBasePos);

		int numGrids = (players[player.playernum]->inventoryUI.MAX_CHEST_Y / getNumItemsToDisplayVertical()) + 1;
		SDL_Rect chestSlotsFramePos = chestSlotsFrame->getSize();

		const int gridHeight = 120 + 2; // 120px is grid img, plus 2px to tile the image for bottom border.

		int heightChange = 0;
		if ( getNumItemsToDisplayVertical() < kNumItemsToDisplayVertical )
		{
			heightChange = player.inventoryUI.getSlotSize() * (kNumItemsToDisplayVertical - getNumItemsToDisplayVertical());
		}
		chestSlotsFramePos.y = 4 + chestBaseImgBorderTopHeight + heightChange + 48;
		chestSlotsFramePos.h = gridHeight - heightChange;
		chestSlotsFrame->setActualSize(SDL_Rect{ chestSlotsFrame->getActualSize().x,
			chestSlotsFrame->getActualSize().y,
			chestSlotsFrame->getActualSize().w,
			(chestSlotsFramePos.h) * numGrids });
		chestSlotsFrame->setSize(chestSlotsFramePos);
		//auto gridImg = chestSlotsFrame->findImage("grid img");
		//gridImg->pos.y = 0;
		//gridImg->pos.h = (chestSlotsFramePos.h) * numGrids;

		/*SDL_Rect sliderPos = slider->getRailSize();
		sliderPos.y = 8 + heightChange + totalFrameHeightChange;
		sliderPos.h = 234 - heightChange;
		slider->setRailSize(sliderPos);*/

		// align title
		auto titleText = baseFrame->findField("title txt");
		auto titlePos = titleText->getSize();
		titlePos.y = baseBackgroundImg->pos.y - 24;
		titlePos.x = baseBackgroundImg->pos.x + baseBackgroundImg->pos.w / 2 - titlePos.w / 2;
		titleText->setSize(titlePos);

		chestFrame->setSize(chestFramePos);
		baseFrame->setSize(chestBasePos);

		// align buttons
		auto grabAllBtnPos = grabAllBtn->getSize();
		auto bg = baseFrame->findImage("chest base img");
		grabAllBtnPos.y = bg->pos.y + bg->pos.h - 4 - grabAllBtnPos.h - 4;
		grabAllBtnPos.x = bg->pos.x + bg->pos.w / 2 - grabAllBtnPos.w / 2;
		grabAllBtn->setSize(grabAllBtnPos);

		auto closeBtnPos = closeBtn->getSize();
		closeBtnPos.x = bg->pos.x + bg->pos.w - closeBtnPos.w - 8 - 4;
		closeBtnPos.y = titlePos.y + titlePos.h / 2 - closeBtnPos.h / 2;
		closeBtn->setSize(closeBtnPos);


		if ( player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_CHEST)
			&& inputs.getVirtualMouse(player.playernum)->draw_cursor && !drawGlyphs )
		{
			if ( !grabAllBtn->isDisabled() && grabAllBtn->isHighlighted() )
			{
				player.GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_CHEST);
				if ( player.GUI.activeModule != Player::GUI_t::MODULE_CHEST )
				{
					player.GUI.activateModule(Player::GUI_t::MODULE_CHEST);
				}
				SDL_Rect pos = grabAllBtn->getAbsoluteSize();
				// make sure to adjust absolute size to camera viewport
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				player.hud.setCursorDisabled(false);
				player.hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player.playernum)->draw_cursor);
			}
			if ( !closeBtn->isDisabled() && closeBtn->isHighlighted() )
			{
				player.GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_CHEST);
				if ( player.GUI.activeModule != Player::GUI_t::MODULE_CHEST )
				{
					player.GUI.activateModule(Player::GUI_t::MODULE_CHEST);
				}
				SDL_Rect pos = closeBtn->getAbsoluteSize();
				// make sure to adjust absolute size to camera viewport
				pos.x -= player.camera_virtualx1();
				pos.y -= player.camera_virtualy1();
				player.hud.setCursorDisabled(false);
				player.hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player.playernum)->draw_cursor);
			}
		}

		int furthestLeftPrompt = xres;
		if ( !promptBack->isDisabled() )
		{
			promptBack->setText(language[4053]);
			SDL_Rect glyphPos = promptBackImg->pos;
			if ( auto textGet = Text::get(promptBack->getText(), promptBack->getFont(),
				promptBack->getTextColor(), promptBack->getOutlineColor()) )
			{
				SDL_Rect textPos = promptBack->getSize();
				textPos.w = textGet->getWidth();
				textPos.x = bg->pos.x + bg->pos.w - textPos.w - 8;
				textPos.y = bg->pos.y + bg->pos.h - textPos.h + 8 + 8;
				promptBack->setSize(textPos);
				glyphPos.x = promptBack->getSize().x + promptBack->getSize().w - textGet->getWidth() - 4;
			}
			promptBackImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
			Image* glyphImage = Image::get(promptBackImg->path.c_str());
			if ( glyphImage )
			{
				glyphPos.w = glyphImage->getWidth();
				glyphPos.h = glyphImage->getHeight();
				glyphPos.x -= glyphPos.w;
				furthestLeftPrompt = std::min(glyphPos.x + glyphPos.w / 2, furthestLeftPrompt);
				glyphPos.y = promptBack->getSize().y + promptBack->getSize().h / 2 - glyphPos.h / 2;
				promptBackImg->pos = glyphPos;
			}
		}
		if ( !promptGrab->isDisabled() )
		{
			promptGrab->setText(language[4091]);
			auto promptGrabImg = baseFrame->findImage("prompt grab img");
			SDL_Rect glyphPos = promptGrabImg->pos;
			if ( auto textGet = Text::get(promptGrab->getText(), promptGrab->getFont(),
				promptGrab->getTextColor(), promptGrab->getOutlineColor()) )
			{
				SDL_Rect textPos = promptGrab->getSize();
				textPos.w = textGet->getWidth();
				textPos.x = bg->pos.x + bg->pos.w - textPos.w - 8;
				textPos.y = promptBack->getSize().y - textPos.h + 2;
				promptGrab->setSize(textPos);
				glyphPos.x = promptGrab->getSize().x + promptGrab->getSize().w - textGet->getWidth() - 4;
			}
			promptGrabImg->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuPageRightAlt");
			Image* glyphImage = Image::get(promptGrabImg->path.c_str());
			if ( glyphImage )
			{
				glyphPos.w = glyphImage->getWidth();
				glyphPos.h = glyphImage->getHeight();
				glyphPos.x -= glyphPos.w;
				furthestLeftPrompt = std::min(glyphPos.x + glyphPos.w / 2, furthestLeftPrompt);
				glyphPos.y = promptGrab->getSize().y + promptGrab->getSize().h / 2 - glyphPos.h / 2;
				promptGrabImg->pos = glyphPos;

				if ( furthestLeftPrompt < promptGrabImg->pos.x + promptGrabImg->pos.w / 2 )
				{
					int offset = (promptGrabImg->pos.x + promptGrabImg->pos.w / 2) - furthestLeftPrompt;
					promptGrabImg->pos.x -= offset;
					auto pos = promptGrab->getSize();
					pos.x -= offset;
					promptGrab->setSize(pos);
				}
			}
		}
		if ( !promptBack->isDisabled() )
		{
			if ( furthestLeftPrompt < promptBackImg->pos.x + promptBackImg->pos.w / 2 )
			{
				int offset = (promptBackImg->pos.x + promptBackImg->pos.w / 2) - furthestLeftPrompt;
				promptBackImg->pos.x -= offset;
				auto pos = promptBack->getSize();
				pos.x = promptGrab->getSize().x;
				promptBack->setSize(pos);
			}
		}
	}

	bool closeChestAction = false;
	if ( !inputs.getUIInteraction(player.playernum)->selectedItem 
		&& player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_CHEST)
		&& !player.GUI.isDropdownActive()
		&& !player.inventoryUI.spellPanel.bOpen
		&& player.bControlEnabled && !gamePaused && !player.usingCommand() )
	{
		if ( openedChest[player.playernum] || bOpen )
		{
			if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
				closeChestAction = true;
			}
			else if ( !promptGrab->isDisabled() && Input::inputs[player.playernum].consumeBinaryToggle("MenuPageRightAlt") )
			{
				takeAllChestGUIAction(player.playernum);
			}
		}
	}

	/*int lowestItemY = getNumItemsToDisplayVertical() - 1;
	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }
		if ( itemCategory(item) == SPELL_CAT ) { continue; }

		lowestItemY = std::max(lowestItemY, item->y);
	}

	int scrollAmount = std::max((lowestItemY + 1) - (getNumItemsToDisplayVertical()), 0) * player.inventoryUI.getSlotSize();
	if ( scrollAmount == 0 )
	{
		slider->setDisabled(true);
	}
	else
	{
		slider->setDisabled(false);
	}

	currentScrollRow = scrollSetpoint / player.inventoryUI.getSlotSize();

	if ( bOpen && isInteractable )
	{
		// do sliders
		if ( !slider->isDisabled() )
		{
			if ( !inputs.getUIInteraction(player.playernum)->selectedItem
				&& player.GUI.activeModule == Player::GUI_t::MODULE_CHEST )
			{
				if ( inputs.bPlayerUsingKeyboardControl(player.playernum) )
				{
					if ( Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] )
					{
						Input::mouseButtons[Input::MOUSE_WHEEL_DOWN] = 0;
						scrollSetpoint = std::max(scrollSetpoint + player.inventoryUI.getSlotSize(), 0);
					}
					if ( Input::mouseButtons[Input::MOUSE_WHEEL_UP] )
					{
						Input::mouseButtons[Input::MOUSE_WHEEL_UP] = 0;
						scrollSetpoint = std::max(scrollSetpoint - player.inventoryUI.getSlotSize(), 0);
					}
				}
				if ( Input::inputs[player.playernum].analogToggle("MenuScrollDown") )
				{
					Input::inputs[player.playernum].consumeAnalogToggle("MenuScrollDown");
					scrollSetpoint = std::max(scrollSetpoint + player.inventoryUI.getSlotSize(), 0);
				}
				else if ( Input::inputs[player.playernum].analogToggle("MenuScrollUp") )
				{
					Input::inputs[player.playernum].consumeAnalogToggle("MenuScrollUp");
					scrollSetpoint = std::max(scrollSetpoint - player.inventoryUI.getSlotSize(), 0);
				}
			}
		}

		scrollSetpoint = std::min(scrollSetpoint, scrollAmount);
		currentScrollRow = scrollSetpoint / player.inventoryUI.getSlotSize();

		if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
		{
			isInteractable = false;
			const real_t fpsScale = (60.f / std::max(1U, fpsLimit));
			real_t setpointDiff = 0.0;
			if ( scrollSetpoint - scrollAnimateX > 0.0 )
			{
				setpointDiff = fpsScale * std::max(3.0, (scrollSetpoint - scrollAnimateX)) / 3.0;
			}
			else
			{
				setpointDiff = fpsScale * std::min(-3.0, (scrollSetpoint - scrollAnimateX)) / 3.0;
			}
			scrollAnimateX += setpointDiff;
			if ( setpointDiff > 0.0 )
			{
				scrollAnimateX = std::min((real_t)scrollSetpoint, scrollAnimateX);
			}
			else
			{
				scrollAnimateX = std::max((real_t)scrollSetpoint, scrollAnimateX);
			}
		}
		else
		{
			scrollAnimateX = scrollSetpoint;
		}
	}

	if ( scrollAmount > 0 )
	{
		slider->setValue((scrollAnimateX / scrollAmount) * 100.0);
	}
	else
	{
		slider->setValue(0.0);
	}*/

	SDL_Rect actualSize = chestSlotsFrame->getActualSize();
	actualSize.y = scrollAnimateX;
	chestSlotsFrame->setActualSize(actualSize);

	if ( closeChestAction )
	{
		closeChestGUIAction(player.playernum);
		return;
	}
}

bool Player::Inventory_t::ChestGUI_t::isSlotVisible(int x, int y) const
{
	if ( player.inventoryUI.chestFrame )
	{
		if ( player.inventoryUI.chestFrame->isDisabled() )
		{
			return false;
		}
	}
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumItemsToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		return true;
	}
	return false;
}

bool Player::Inventory_t::ChestGUI_t::isItemVisible(Item* item) const
{
	if ( !item ) { return false; }
	return isSlotVisible(item->x, item->y);
}

void Player::Inventory_t::ChestGUI_t::scrollToSlot(int x, int y, bool instantly)
{
	int lowerY = currentScrollRow;
	int upperY = currentScrollRow + getNumItemsToDisplayVertical() - 1;

	if ( y >= lowerY && y <= upperY )
	{
		// no work to do.
		return;
	}

	int lowestItemY = getNumItemsToDisplayVertical() - 1;
	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item ) { continue; }
		if ( itemCategory(item) != SPELL_CAT ) { continue; }

		lowestItemY = std::max(lowestItemY, item->y);
	}
	int maxScroll = std::max((lowestItemY + 1) - (getNumItemsToDisplayVertical()), 0) * player.inventoryUI.getSlotSize();

	int scrollAmount = 0;
	if ( y < lowerY )
	{
		scrollAmount = (y)* player.inventoryUI.getSlotSize();
		//scrollAmount += scrollSetpoint;
	}
	else if ( y > upperY )
	{
		scrollAmount = (y - upperY) * player.inventoryUI.getSlotSize();
		scrollAmount += scrollSetpoint;
	}
	scrollAmount = std::min(scrollAmount, maxScroll);

	scrollSetpoint = scrollAmount;
	if ( instantly )
	{
		scrollAnimateX = scrollSetpoint;
	}
	currentScrollRow = scrollSetpoint / player.inventoryUI.getSlotSize();
	if ( abs(scrollSetpoint - scrollAnimateX) > 0.00001 )
	{
		isInteractable = false;
	}
}