local Bullet = {}
Bullet.__index = Bullet

-- Load sprites once (class-level)
Bullet.sprites = nil
Bullet.animationSets = nil

function Bullet.loadSprites()
    if Bullet.sprites then
        return
    end

    Bullet.sprites = {}
    -- load sprites 1-20
    for i = 1, 20 do
        local path = string.format("sprites/projectile/LightEffect_%02d.png", i)
        Bullet.sprites[i] = love.graphics.newImage(path)
    end

    -- define 4 animation sets
    Bullet.animationSets = {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {16, 17, 18, 19, 20}}
end

function Bullet.new(x, y, vx, vy, maxBounces)
    Bullet.loadSprites()

    local self = setmetatable({}, Bullet)
    self.x = x
    self.y = y
    self.width = 8
    self.height = 8
    self.vx = vx
    self.vy = vy
    self.speed = 250

    -- normalize
    local magnitude = math.sqrt(vx * vx + vy * vy)
    if magnitude > 0 then
        self.vx = (vx / magnitude) * self.speed
        self.vy = (vy / magnitude) * self.speed
    end

    self.maxBounces = maxBounces or 3
    self.bounceCount = 0
    self.active = true

    -- animation
    self.animationSet = Bullet.animationSets[love.math.random(1, 4)]
    self.animationSequence = {1, 2, 3, 4, 5, 4, 3, 2} --  (1->2->3->4->5->4->3->2->1)
    self.animationIndex = 1
    self.animationTimer = 0
    self.animationSpeed = 0.08 -- seconds per frame
    self.scale = 2.7

    return self
end

function Bullet:update(dt, gameWidth, gameHeight)
    if not self.active then
        return
    end

    self.animationTimer = self.animationTimer + dt
    if self.animationTimer >= self.animationSpeed then
        self.animationTimer = self.animationTimer - self.animationSpeed
        self.animationIndex = self.animationIndex + 1
        if self.animationIndex > #self.animationSequence then
            self.animationIndex = 1
        end
    end

    -- update position
    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt
    local bounced = false

    if self.x < 0 then
        self.x = 0
        self.vx = math.abs(self.vx)
        bounced = true
    elseif self.x + self.width > gameWidth then
        self.x = gameWidth - self.width
        self.vx = -math.abs(self.vx)
        bounced = true
    end

    if self.y < 0 then
        self.y = 0
        self.vy = math.abs(self.vy)
        bounced = true
    elseif self.y + self.height > gameHeight then
        self.y = gameHeight - self.height
        self.vy = -math.abs(self.vy)
        bounced = true
    end

    -- count bounces
    if bounced then
        self.bounceCount = self.bounceCount + 1
        if self.bounceCount > self.maxBounces then
            self.active = false
        end
    end
end

function Bullet:draw()
    if not self.active then
        return
    end

    local frameIndex = self.animationSequence[self.animationIndex]
    local spriteIndex = self.animationSet[frameIndex]
    local sprite = Bullet.sprites[spriteIndex]

    if sprite then
        local centerX = self.x + self.width / 2
        local centerY = self.y + self.height / 2

        local spriteWidth = sprite:getWidth()
        local spriteHeight = sprite:getHeight()
        local targetSize = math.max(self.width, self.height) * self.scale
        local scaleX = targetSize / spriteWidth
        local scaleY = targetSize / spriteHeight
        local finalScale = math.min(scaleX, scaleY)

        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.draw(sprite, centerX, centerY, 0, finalScale, finalScale, spriteWidth / 2, spriteHeight / 2)
    else
        love.graphics.setColor(1, 1, 0)
        love.graphics.rectangle('fill', self.x, self.y, self.width, self.height)
    end

    love.graphics.setColor(1, 1, 1)
end

function Bullet:isActive()
    return self.active
end

function Bullet:getCollisionBox()
    return self.x, self.y, self.width, self.height
end

return Bullet
