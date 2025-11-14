local Player = {}
Player.__index = Player

Player.sprites = nil

function Player.loadSprites()
    if Player.sprites then
        return
    end

    Player.sprites = {}
    for i = 1, 4 do
        local path = string.format("sprites/player/p_0%d.png", i)
        Player.sprites[i] = love.graphics.newImage(path)
    end
end

function Player.new(x, y)
    Player.loadSprites()

    local self = setmetatable({}, Player)
    self.x = x
    self.y = y
    self.width = 20
    self.height = 20

    self.baseSpeed = 300
    self.speed = self.baseSpeed
    -- animation
    self.currentFrame = 1
    self.animationTimer = 0
    self.animationSpeed = 0.1 -- seconds per frame

    return self
end

function Player:update(dt, gameWidth, gameHeight)
    -- update animation
    self.animationTimer = self.animationTimer + dt
    if self.animationTimer >= self.animationSpeed then
        self.animationTimer = self.animationTimer - self.animationSpeed
        self.currentFrame = self.currentFrame + 1
        if self.currentFrame > 4 then
            self.currentFrame = 1
        end
    end

    local dx = 0
    local dy = 0

    if love.keyboard.isDown('w') or love.keyboard.isDown('up') then
        dy = dy - 1
    end
    if love.keyboard.isDown('s') or love.keyboard.isDown('down') then
        dy = dy + 1
    end
    if love.keyboard.isDown('a') or love.keyboard.isDown('left') then
        dx = dx - 1
    end
    if love.keyboard.isDown('d') or love.keyboard.isDown('right') then
        dx = dx + 1
    end

    -- normalize
    if dx ~= 0 or dy ~= 0 then
        local magnitude = math.sqrt(dx * dx + dy * dy)
        dx = dx / magnitude
        dy = dy / magnitude
    end

    self.x = self.x + dx * self.speed * dt
    self.y = self.y + dy * self.speed * dt

    -- clamp 
    if self.x < 0 then
        self.x = 0
    elseif self.x + self.width > gameWidth then
        self.x = gameWidth - self.width
    end

    if self.y < 0 then
        self.y = 0
    elseif self.y + self.height > gameHeight then
        self.y = gameHeight - self.height
    end
end

function Player:draw()
    local sprite = Player.sprites[self.currentFrame]

    if sprite then
        local centerX = self.x + self.width / 2
        local centerY = self.y + self.height / 2

        local spriteWidth = sprite:getWidth()
        local spriteHeight = sprite:getHeight()
        local scaleX = self.width / spriteWidth
        local scaleY = self.height / spriteHeight
        local scale = math.min(scaleX, scaleY)

        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.draw(sprite, centerX, centerY, 0, scale, scale, spriteWidth / 2, spriteHeight / 2)
    else
        love.graphics.setColor(0, 1, 0)
        love.graphics.rectangle('fill', self.x, self.y, self.width, self.height)
    end

    love.graphics.setColor(1, 1, 1)
end

function Player:getCollisionBox()
    return self.x, self.y, self.width, self.height
end

return Player
