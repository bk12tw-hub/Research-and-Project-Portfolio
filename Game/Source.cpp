#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#define Max_number_of_item 3
using namespace std;
using namespace sf;

int main()
{
    RenderWindow window(sf::VideoMode(800, 600), "SFML Jumping Character");

    //View
    View view(sf::FloatRect(0, 0, 800, 600));
    window.setView(view);
    
    //有夠多的Texture
    Texture lastOne;
    Texture texture, mountainsTexture,texture7,t8,t9,t10,MapTexture,bridgeTexture,Enemy2Texture, EndTexture,MountainsMidTexture;
    if (!texture.loadFromFile("sprite.png")) {
        return -1; // 圖像載入失敗，退出程序
    }
    //Music
    sf::Music music;
    if (!music.openFromFile("music.ogg"))
    {
        return -1;// 处理加载音乐失败的情况
    }
    music.setLoop(true);

    MapTexture.loadFromFile("map.png"); lastOne.loadFromFile("weird_knight2.png");
    mountainsTexture.loadFromFile("mountains.png"); texture7.loadFromFile("sky.png");
    t8.loadFromFile("land.png"); t9.loadFromFile("sword.png"); t10.loadFromFile("knight.png"); 
    bridgeTexture.loadFromFile("bridge.png"); Enemy2Texture.loadFromFile("weird_knight.png");
    EndTexture.loadFromFile("mountains2.png"); MountainsMidTexture.loadFromFile("mountains_mid.png");
    //場景設置
    Sprite Background(MapTexture);                         Sprite Background2(MapTexture);
    Background.setPosition(-4, 540);                       Background2.setPosition(2770, 540);
    Background.setTextureRect(IntRect(0, 50, 700, 20));
    Background.setScale(sf::Vector2f(4.f, 3.f));           Background2.setTextureRect(IntRect(0, 0, 700, 20));
                                                           Background2.setScale(sf::Vector2f(4.f, 3.f));

    Sprite Mountains(mountainsTexture);                    Sprite Mountains2(mountainsTexture);
    Mountains.setPosition(0, -200);                          Mountains2.setPosition(2780, -200);
    Mountains.setScale(sf::Vector2f(5.f, 5.f));            Mountains2.setScale(sf::Vector2f(5.f, 5.f));

    Sprite SKY(texture7);                       Sprite SKY2(texture7);
    SKY.setPosition(0, -220);                    SKY2.setPosition(3530, -230);
    SKY.setScale(sf::Vector2f(5.5f, 3.f));       SKY2.setScale(sf::Vector2f(5.5f, 3.f));

    Sprite SKY3(texture7);
    SKY3.setPosition(7000, -240);
    SKY3.setScale(sf::Vector2f(5.5f, 3.f));

    Sprite Mountains3(MountainsMidTexture);
    Mountains3.setPosition(6130, -150);
    Mountains3.setScale(sf::Vector2f(3.5f, 3.5f));

    Sprite Endmap(EndTexture);
    Endmap.setPosition(7600, -200);
    Endmap.setScale(sf::Vector2f(5.f, 5.f));

    Sprite Land(t8);
    Land.setPosition(5800,450 );
    Land.setScale(sf::Vector2f(4.f, 4.f));

    Sprite Land2(t8);
    Land2.setPosition(6600, 400);
    Land2.setScale(sf::Vector2f(4.f, 4.f));

    Sprite Bridge(bridgeTexture);
    Bridge.setPosition(7000, 350);
    Bridge.setTextureRect(IntRect(0, 55, 651, 150));
    Bridge.setScale(sf::Vector2f(4.f, 4.f));

    Sprite Bridge_2(bridgeTexture);
    Bridge_2.setPosition(7100, 250);
    Bridge_2.setTextureRect(IntRect(0, 15, 651, 40));
    Bridge_2.setScale(sf::Vector2f(3.f, 3.f));

    //道具
    Sprite Sword(t9);
    //Sword.setPosition(5500, 545);
    Sword.setPosition(400, 545);
    Sword.setOrigin(32, 140);
    Sword.setRotation(55);
    Sword.setScale(sf::Vector2f(0.6f, 0.6f));
    
    bool isPickedUp=false;
    bool isSwinging=false;

    Sprite Pizza(MapTexture);
    Pizza.setTextureRect(IntRect(482, 164, 18, 18));
    Pizza.setScale(sf::Vector2f(3.5f, 3.5f));

    bool PizzaisPickedUp = false;
    bool PizzaisSwinging = false;
    bool isPickedUp3 = false;
    bool isSwinging3 = false;
    //不知道為甚麼但就是想放顆tree
    Sprite Tree(MapTexture);
    Tree.setPosition(150, 270);
    Tree.setTextureRect(IntRect(180, 70, 105, 140));
    Tree.setScale(sf::Vector2f(2.f, 2.f));

    //Enemy設定
    Sprite Enemy(t10);
    //Enemy.setPosition(500, 440);
    Enemy.setPosition(7200,220);
    Enemy.setTextureRect(IntRect(0, 0, 15, 20));
    Enemy.setScale(sf::Vector2f(5.5f, 5.5f));
    bool notDestroyed=true;
    bool eneyisattacking = false;
    int enemydirection=1;
    sf::RectangleShape EneyBlood;
    EneyBlood.setFillColor(sf::Color::Red);
    EneyBlood.setSize(sf::Vector2f(50, 2));

    Sprite enemy_sword(t10);
    enemy_sword.setTextureRect(IntRect(43, 4, 13, 14));
    enemy_sword.setScale(sf::Vector2f(5.5f, 5.5f));
    enemy_sword.setOrigin(0, 10);

    //Enemy2
    Sprite Enemy2(Enemy2Texture);
    Enemy2.setPosition(1800, 450);
    //Enemy2.setPosition(400, 450);
    Enemy2.setTextureRect(IntRect(0, 0, 15, 20));
    Enemy2.setScale(sf::Vector2f(4.f, 4.f));
    bool notDestroyed2 = true;
    bool eneyisattacking2 = false;
    int enemydirection2 = 1;
    sf::RectangleShape Eney2Blood;
    Eney2Blood.setFillColor(sf::Color::Red);
    Eney2Blood.setSize(sf::Vector2f(25, 2));

    //Enemy3
    Sprite Enemy3(lastOne);
    Enemy3.setPosition(4000, 450);
    Enemy3.setTextureRect(IntRect(0, 0, 15, 20));
    Enemy3.setScale(sf::Vector2f(4.f, 4.f));
    bool notDestroyed3 = true;
    bool eneyisattacking3 = false;
    int enemydirection3 = 1;
    sf::RectangleShape Eney3Blood;
    Eney3Blood.setFillColor(sf::Color::Red);
    Eney3Blood.setSize(sf::Vector2f(30, 2));
    
    //角色設定
    Sprite character(texture);
    character.setPosition(0, 0);
    character.setTextureRect(IntRect(0, 0, 20, 24));
    character.setScale(sf::Vector2f(4.f, 4.f));

    Sprite characterblood(MapTexture);
    characterblood.setPosition(10, 10);
    characterblood.setTextureRect(IntRect(483, 76, 36, 8));
    characterblood.setScale(sf::Vector2f(5.f, 5.f));
    bool character_is_being_attacking = false;
    bool characterisHolding=false;

    //說明Text
    Font font;
    font.loadFromFile("font.ttf");
    Text descriptionText, descriptionText2,characterText,EndText, operationText, descriptionText3;
    Text MenuText[Max_number_of_item];
    descriptionText.setFont(font);
    descriptionText.setCharacterSize(23);
    descriptionText.setFillColor(sf::Color::White);//Sword的說明

    descriptionText2.setFont(font);
    descriptionText2.setCharacterSize(23);
    descriptionText2.setFillColor(sf::Color::White);//Pizza的說明

    descriptionText3.setFont(font);
    descriptionText3.setCharacterSize(23);
    descriptionText3.setFillColor(sf::Color::White);//CPLD的說明

    characterText.setFont(font);
    characterText.setCharacterSize(20);
    characterText.setFillColor(sf::Color::Black);//角色的自言自語pt1
    bool alongtime = false;

    operationText.setFont(font);
    operationText.setPosition(0,230);
    operationText.setCharacterSize(30);
    operationText.setString(L"'W'跳躍\n'D'前進\n'A'後退\n'S+D'撿拾物品\n'J'攻擊");
    operationText.setFillColor(sf::Color::Cyan);//反正會被忽略的操作說明

    MenuText[2].setFont(font);
    MenuText[2].setCharacterSize(40);
    MenuText[2].setString("Play");
    MenuText[2].setFillColor(sf::Color::Red);
    MenuText[2].setPosition(800 / 2, window.getSize().y / (Max_number_of_item + 1));

    MenuText[1].setFont(font);
    MenuText[1].setCharacterSize(40);
    MenuText[1].setString("Still Play");
    MenuText[1].setFillColor(sf::Color::White);
    MenuText[1].setPosition(800 / 2, window.getSize().y / (Max_number_of_item + 1) * 2);
    int select=2;
    bool isBeingSelected = false;

    EndText.setFont(font);
    EndText.setCharacterSize(40);
    EndText.setString(L"恭喜過關！");
    EndText.setFillColor(sf::Color::Yellow);//有夠快出現的過關文字
    bool finished=false;

    // 跳躍速度             // 重力加速度;
    float jumpSpeed = 0.2f,gravity = 0.00012f;//0.0006   
    // 垂直速度             // 鉛直速度
    float yVelocity = 0.0f,xVelocity = 0.0f;
    float movespeed = 0.05f;   //移動速動
    float currentframe = 0.f, currentframe2 = 0.f, currentframes3 = 0.f, currentframes4=0.f, currentframes5 = 0.f;   //動畫專區//跑步動畫//揮動動畫
    int direction=0;            //偵測character面向左邊還是右邊
    float counter = 0.f, counter2 = 0.f, counter3 = 0.f, counter4 = 0.f, counter5 = 0.f, counter6 = 0,counter7 = 0;
    float counter8 = 0.f,counter9=0.f;
    bool Playerwanttoplay=false;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {

            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }
        if(Playerwanttoplay){

           // music.play();
            music.setVolume(70);
        // 模擬重力對角色的影響
        yVelocity += gravity;
        //精美的血條設定
        if(character.getPosition().x < 390)
            characterblood.setPosition(10, 10);
        else
            characterblood.setPosition(character.getPosition().x-360, 10);
        //一大堆的地圖碰撞
        if(character.getGlobalBounds().intersects(Land.getGlobalBounds())) {
            character.setPosition(character.getPosition().x, Land.getPosition().y - character.getGlobalBounds().height);
            yVelocity = 0.0f;
            if (character.getGlobalBounds().left < Land.getGlobalBounds().left) {
            xVelocity = 0.0f;
            character.setPosition(Land.getGlobalBounds().left - character.getGlobalBounds().width, character.getPosition().y);
            }
        }
        if (character.getGlobalBounds().intersects(Land2.getGlobalBounds())) {
            character.setPosition(character.getPosition().x, Land2.getPosition().y - character.getGlobalBounds().height);
            yVelocity = 0.0f;
        }
        if (character.getGlobalBounds().intersects(Bridge.getGlobalBounds())) {
            character.setPosition(character.getPosition().x, Bridge.getPosition().y - character.getGlobalBounds().height);
            yVelocity = 0.0f;
        }
        if (character.getGlobalBounds().intersects(Background.getGlobalBounds())|| character.getGlobalBounds().intersects(Background2.getGlobalBounds())|| character.getGlobalBounds().intersects(Land.getGlobalBounds())) {
            character.setPosition(character.getPosition().x, Background.getPosition().y - character.getGlobalBounds().height);
            yVelocity = 0.0f;
        }
        // 檢查角色和道具的碰撞 / 華麗的文字特效
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { 
            if(character.getGlobalBounds().intersects(Sword.getGlobalBounds()))
            isPickedUp = true;
            if (character.getGlobalBounds().intersects(Pizza.getGlobalBounds()))
            PizzaisPickedUp = true;
            descriptionText.setPosition(Sword.getPosition().x-130, Sword.getPosition().y - 150);
        }
        //披薩部分
        if (!PizzaisPickedUp) {
            descriptionText2.setString(L"導生聚吃剩的披薩");
            descriptionText2.setPosition(Pizza.getPosition().x-60, Pizza.getPosition().y-50);
        }
        if (character.getGlobalBounds().intersects(Pizza.getGlobalBounds()) && PizzaisPickedUp) {
            if (!isPickedUp) {
                if (direction == 1) {
                    Pizza.setTextureRect(IntRect(482, 182, 18, 20));
                    Pizza.setPosition(character.getPosition().x - 50, character.getPosition().y + 5);
                }
                else {
                    Pizza.setTextureRect(IntRect(482, 164, 18, 18));
                    Pizza.setPosition(character.getPosition().x + 40, character.getPosition().y);
                }
            }
            else if(isPickedUp){
                isPickedUp = false;
                Sword.setPosition(character.getPosition().x,character.getPosition().y+80);
            }
        }
        //匕首部分
        if(!isPickedUp){    
            descriptionText.setString(L"不知道被誰遺棄在路邊的匕首");
            descriptionText.setPosition(Sword.getPosition().x-130, Sword.getPosition().y-120);
        }
        if (character.getGlobalBounds().intersects(Sword.getGlobalBounds())&&isPickedUp) {
           if(!PizzaisPickedUp){
            if (direction == 1) {
                Sword.setRotation(-45);
                Sword.setPosition(character.getPosition().x+23, character.getPosition().y+65);
            }
            else {
                Sword.setRotation(45);
                Sword.setPosition(character.getPosition().x+37 , character.getPosition().y+65);
            }
            //有夠搞工的attack，做完瞬間覺得自己無所不能
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) {
                isSwinging = true;
            }
            if (isSwinging) {
                currentframe2 += 0.07; 
                if(direction==1){
                    Sword.setRotation(-45 - currentframe2);
                    Sword.setPosition(character.getPosition().x + 30, character.getPosition().y + 60-0.2*currentframe2);
                }
                else {
                    Sword.setRotation(45 + currentframe2);
                    Sword.setPosition(character.getPosition().x + 30, character.getPosition().y + 60 - 0.2 * currentframe2);
                }
                if (currentframe2 > 80) {
                    isSwinging = false;
                    currentframe2 -= 80;
                }
              }
           }
           else {
                PizzaisPickedUp = false;
                Pizza.setPosition(character.getPosition().x, character.getPosition().y+80);
            }
        }
        //attack Enemy
        if(!notDestroyed&&!notDestroyed2&&!notDestroyed3)  finished = true;
        if (Enemy.getGlobalBounds().intersects(Sword.getGlobalBounds())&&isSwinging|| Enemy2.getGlobalBounds().intersects(Sword.getGlobalBounds()) && isSwinging|| Enemy3.getGlobalBounds().intersects(Sword.getGlobalBounds()) && isSwinging) {
            if(Enemy.getGlobalBounds().intersects(Sword.getGlobalBounds())){
                counter+=0.01f;
                EneyBlood.setSize((sf::Vector2f(55-1.5*int(counter), 2)));
            }else if(Enemy2.getGlobalBounds().intersects(Sword.getGlobalBounds())){
                counter8 += 0.01;
                Eney2Blood.setSize((sf::Vector2f(31 - 1.5*int(counter8), 2)));
            }
            else {
                counter9+=0.01;
                Eney3Blood.setSize((sf::Vector2f(45 - 1.5 *int(counter9), 2)));
            }
            if (counter > 37) {
                counter -= 37;
                notDestroyed = false;
            }
            if (counter8 > 23) {
                counter8 -= 23;
                notDestroyed2 = false;
            }
            if (counter9 > 30) {
                counter9 -= 30;
                notDestroyed3 = false;
            }
        }//Enemy水平移動
        else {
            if (!notDestroyed) { 
                Enemy.move(0, 0); 
                if(!PizzaisPickedUp)
                Pizza.setPosition(Enemy.getPosition().x , Enemy.getPosition().y+50);
            }
            else if (Enemy.getGlobalBounds().intersects(character.getGlobalBounds())|| Enemy2.getGlobalBounds().intersects(character.getGlobalBounds())|| Enemy3.getGlobalBounds().intersects(character.getGlobalBounds())) {
                Enemy.move(0,0);
                Enemy2.move(0, 0);
                Enemy3.move(0, 0);
                eneyisattacking = true;
                eneyisattacking2 = true;
                eneyisattacking3 = true;// 
            }
           else {
                Enemy.move(0.05 * enemydirection, 0);
                Enemy2.move(0.05 * enemydirection2, 0);
                Enemy3.move(0.05 * enemydirection3, 0);
            }
            //敵人3:小紫人
            if (Enemy3.getPosition().x > 5000) {    //1000
                enemydirection3 = -1;
            }
            if (Enemy3.getPosition().x < 3000) {    //800
                enemydirection3 = 1;
            }
            if (enemydirection3 == -1) {
                currentframes5 += 0.00125;
                Enemy3.setTextureRect(IntRect(48 - 16 * int(currentframes5), 91, 15, 23));
                if (currentframes5 > 4)
                    currentframes5 -= 4;
            }
            else {
                currentframes5 += 0.00125;
                Enemy3.setTextureRect(IntRect(16 * int(currentframes5), 46, 15, 23));
                if (currentframes5 > 4)
                    currentframes5 -= 4;
            }
            if (!notDestroyed3) {
                eneyisattacking3 = false;
            }
            //敵人2:黏黏怪
            if (Enemy2.getPosition().x > 2400) {    //1000
                enemydirection2 = -1;
            }
            if (Enemy2.getPosition().x < 1600) {    //800
                enemydirection2 = 1;
            }            
            if (enemydirection2 == -1) {
                currentframes4 += 0.03125;
                Enemy2.setTextureRect(IntRect(48 - 16 * int(currentframe), 93, 15, 30));
                if (currentframes4 > 4)
                    currentframes4 -= 4;
            }
            else {
                currentframes4 += 0.03125;
                Enemy2.setTextureRect(IntRect(16 * int(currentframe), 46, 15, 23));
                if (currentframes4 > 4)
                    currentframes4 -= 4;
            }
            if (!notDestroyed2) {
                eneyisattacking2 = false;
            }
            //敵人1:Big Boss
            if (Enemy.getPosition().x > 8000) {    //7700
                enemydirection = -1;
            }
            if (Enemy.getPosition().x < 6950) {    //7000
                enemydirection = 1;}
            if (enemydirection == -1) {
                currentframes3 += 0.000625;
                enemy_sword.setRotation(-110);
                enemy_sword.setPosition(Enemy.getPosition().x+10, Enemy.getPosition().y+100);
                Enemy.setTextureRect(IntRect(50 - 16 * int(currentframe), 93, 15, 30));
                if (currentframes3 > 4)
                    currentframes3 -= 4;
            }
            else {
                currentframes3 += 0.000625;
                enemy_sword.setRotation(30);
                enemy_sword.setPosition(Enemy.getPosition().x+55, Enemy.getPosition().y+70);
                Enemy.setTextureRect(IntRect(16 * int(currentframe),46, 15, 25));
                if (currentframes3 > 4)
                    currentframes3 -= 4;
            }
        }//敵人存在的狀態
        if (notDestroyed) {
            if (eneyisattacking) {
                counter4 += 0.03;
                if (enemydirection == 1) {//Right
                    enemy_sword.setRotation(30 + counter4);
                    enemy_sword.setPosition(Enemy.getPosition().x + 45 + counter4, Enemy.getPosition().y + 65);
                }
                else {//Left
                    enemy_sword.setRotation(-100 - counter4);
                    enemy_sword.setPosition(Enemy.getPosition().x + 10 - counter4, Enemy.getPosition().y + 90);
                }
                if (counter4 > 25) {
                    counter4 -= 25;
                    eneyisattacking = false;
                }
            }
        //角色被攻擊的部分
        if (character.getGlobalBounds().intersects(enemy_sword.getGlobalBounds()) && eneyisattacking && int(counter7)%70==0|| eneyisattacking2 &&character.getGlobalBounds().intersects(Enemy2.getGlobalBounds()) && int(counter7) % 70 == 0|| character.getGlobalBounds().intersects(Enemy3.getGlobalBounds()) && eneyisattacking3 && int(counter7) % 70 == 0) {
            character_is_being_attacking = true;
            counter5 += 0.5;
            if (direction == 1)
                character.setPosition(character.getPosition().x + 30 * counter5, character.getPosition().y-10);
            else
                character.setPosition(character.getPosition().x - 30 * counter5, character.getPosition().y-10);
            if (counter5 > 20) {
                counter5 -= 20;
                character_is_being_attacking = false;
            }
        }
        else 
            counter7 += 0.025;
       //這裡處理的是角色的血量
        if (character_is_being_attacking) {
            counter6 += 1;
            character_is_being_attacking = false;
            if (counter6 == 1)    characterblood.setTextureRect(IntRect(483, 76, 36, 8));
            if (counter6 == 2)    characterblood.setTextureRect(IntRect(483, 84, 36, 8));
            if (counter6 == 3)    characterblood.setTextureRect(IntRect(483, 93, 36, 8));
            if (counter6 == 4)    characterblood.setTextureRect(IntRect(483, 110, 36, 8));
            if (counter6 == 5)    characterblood.setTextureRect(IntRect(483, 119, 36, 8));
        }
        }
        // 檢測是否按下空白鍵，如果是則讓角色跳躍
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
            if (yVelocity == 0.0f)
            {
                yVelocity = -jumpSpeed;
            }
        }
        //檢測是否按下左/右鍵，如果是則讓角色水平移動
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            currentframe += 0.002f;
            xVelocity = movespeed; 
            direction = 0;
            character.setTextureRect(IntRect(0, 0, 20, 24));
            //終於成功的角色移動動畫
            character.setTextureRect(IntRect(16* int(currentframe), 50, 15,22 ));
            if (currentframe > 4) 
                currentframe -= 4;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            currentframe += 0.002f;
            xVelocity = -movespeed;
            direction = 1;
            character.setTextureRect(IntRect(20, 0, 20, 24));
            character.setTextureRect(IntRect(48 -16* int(currentframe), 95, 15, 22));
            if (currentframe > 4)
                currentframe -= 4;
        }
        else { 
            xVelocity = 0; 
            if (direction == 0) {
                currentframe += 0.001f;
                character.setTextureRect(IntRect(16 * int(currentframe), 29, 15, 19));
                if (currentframe > 4)
                    currentframe -= 4;
            }
            else {
            currentframe += 0.001f;
            character.setTextureRect(IntRect(48- 16 * int(currentframe), 74, 15, 19));
            if (currentframe > 4)
                currentframe -= 4;
            }
        }
        //character的自言自語
        counter3 += 0.001f;
        if (counter3 > 100) { 
            alongtime = false;
            characterText.setString(L"聽說北海住著一種神獸，叫作玄武，你聽過嗎？");
            characterText.setPosition(character.getPosition().x - 150, character.getPosition().y - 50);
            if (counter3 > 120) {
                alongtime = true;
                counter3 -= 120;
            }
        }
        // 增加邊界檢查
        if (character.getPosition().x <= 0) {
            character.setPosition(0, character.getPosition().y);
        }
        else if(character.getPosition().y>=window.getSize().y||counter6==5){
            xVelocity = 0; yVelocity = 0;
            if (!notDestroyed && !notDestroyed2) 
                cout << "Game Complete！";
            else
            cout << "Game Over";
            return -1;
        }
        else {
            character.move(xVelocity, 0.0f);
        }
                window.setView(view);
                // 如果角色碰到了視窗的邊界，限制視角的移動
                if (view.getCenter().x < 800 / 2) {
                    view.setCenter(800 / 2, 600 / 2);
                }
                else if (view.getCenter().y > 600 / 2) {
                    view.setCenter(character.getPosition().x + character.getGlobalBounds().width / 2,600/2);
                }
        window.setView(view);
        character.move(xVelocity, yVelocity);
        // 設定視角的中心點為角色的位置
        view.setCenter(character.getPosition().x + character.getGlobalBounds().width / 2, character.getPosition().y );

        window.clear(sf::Color::White);
        window.draw(SKY); window.draw(SKY2);   window.draw(SKY3);   
        window.draw(Mountains);     window.draw(Mountains2);    window.draw(Endmap);         window.draw(Mountains3);
        window.draw(Tree);          window.draw(Bridge);      
        window.draw(Background);    window.draw(Background2);
        window.draw(Land);          window.draw(Land2);
        if (notDestroyed) {
            window.draw(EneyBlood);
            EneyBlood.setPosition(Enemy.getPosition().x+10, Enemy.getPosition().y);
            window.draw(enemy_sword);
            window.draw(Enemy);
        }else Enemy.move(0, 0);
        if (notDestroyed2) {
            window.draw(Enemy2);
            Eney2Blood.setPosition(Enemy2.getPosition().x+10, Enemy2.getPosition().y);
            window.draw(Eney2Blood);

        }
        if (notDestroyed3) {
                window.draw(Enemy3);
                Eney3Blood.setPosition(Enemy3.getPosition().x + 10, Enemy3.getPosition().y);
                window.draw(Eney3Blood);

        }
        window.draw(Sword); 
        if (!notDestroyed) {
           window.draw(Pizza);
            if (!PizzaisPickedUp)
                window.draw(descriptionText2);
        }
        //window.draw(Computerboard);
        window.draw(character);
        window.draw(Bridge_2);
        window.draw(characterblood);
        if(!alongtime)
            window.draw(characterText);
        if (!isPickedUp)
            window.draw(descriptionText);
        if (finished) {
            EndText.setPosition(character.getPosition().x-50, character.getPosition().y - 150);
            window.draw(EndText);     
        }

        window.draw(operationText);
        window.display();
        }
        else {
            music.play();
            window.clear(sf::Color::Black);
            for (int i = 0; i < Max_number_of_item; i++) {
                window.draw(MenuText[i]);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                if (select < 2) {
                    select++;
                    MenuText[select].setFillColor(Color::Red);
                }
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
               if (select > 1) {
                   select--;
                   MenuText[select].setFillColor(Color::Red);
               }
            }
            else {
                MenuText[3-select].setFillColor(Color::White);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter))
                Playerwanttoplay = true;
          window.display();
        }
    }
    return 0;
}