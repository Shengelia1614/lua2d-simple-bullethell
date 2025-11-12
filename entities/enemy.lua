local Enemy = {}
Enemy.__index = Enemy

Enemy.sprites = nil

function Enemy.loadSprites()
    if Enemy.sprites then
        return
    end

    Enemy.sprites = {}
    for i = 1, 48 do
        local path = string.format("sprites/enemy/out_hjm-charged_bolt_v4_%02d.png", i)
        Enemy.sprites[i] = love.graphics.newImage(path)
    end
end

function Enemy.new(x, y, vx, vy)
    Enemy.loadSprites()

    local self = setmetatable({}, Enemy)
    self.x = x
    self.y = y
    self.width = 60 -- Twice the original size (30 * 2)
    self.height = 60
    self.vx = vx or 150
    self.vy = vy or 100
    self.shootTimer = 0
    self.shootInterval = 1.0 -- shoots evrey   second

    -- animation
    self.currentFrame = 1
    self.animationTimer = 0
    self.animationSpeed = 0.05 -- seconds per frame

    return self
end

function Enemy:update(dt, gameWidth, gameHeight)
    self.animationTimer = self.animationTimer + dt
    if self.animationTimer >= self.animationSpeed then
        self.animationTimer = self.animationTimer - self.animationSpeed
        self.currentFrame = self.currentFrame + 1
        if self.currentFrame > 48 then
            self.currentFrame = 1
        end
    end

    -- updateposition
    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt

    -- bounces off walls
    if self.x < 0 then
        self.x = 0
        self.vx = math.abs(self.vx)
    elseif self.x + self.width > gameWidth then
        self.x = gameWidth - self.width
        self.vx = -math.abs(self.vx)
    end

    if self.y < 0 then
        self.y = 0
        self.vy = math.abs(self.vy)
    elseif self.y + self.height > gameHeight then
        self.y = gameHeight - self.height
        self.vy = -math.abs(self.vy)
    end

    -- update shoot
    self.shootTimer = self.shootTimer + dt
end

function Enemy:shouldShoot()
    if self.shootTimer >= self.shootInterval then
        self.shootTimer = self.shootTimer - self.shootInterval
        return true
    end
    return false
end

function Enemy:getCenter()
    return self.x + self.width / 2, self.y + self.height / 2
end

function Enemy:draw()
    local sprite = Enemy.sprites[self.currentFrame]

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
        love.graphics.setColor(1, 0, 0)
        love.graphics.rectangle('fill', self.x, self.y, self.width, self.height)
    end

    love.graphics.setColor(1, 1, 1)
end

function Enemy:getCollisionBox()
    return self.x, self.y, self.width, self.height
end

return Enemy
