local Player = require('entities.player')
local Enemy = require('entities.enemy')
local Bullet = require('entities.bullet')
local Barrier = require('entities.barrier')
local Collision = require('utils.collision')

local PlayingState = {}

function PlayingState:enter()
    self.player = Player.new(400 - 10, 300 - 10)
    self.enemy = Enemy.new(200, 150, 150, 100)
    self.barrier = Barrier.new(self.player)
    self.bullets = {}
    self.gameTime = 0
    self.currentMaxBounces = 3
    self.nextBounceIncrement = 30
end

function PlayingState:update(dt)
    -- update game time
    self.gameTime = self.gameTime + dt

    -- check if we need to increment max bounces (every 2 minutes)

    if self.gameTime >= self.nextBounceIncrement then
        self.currentMaxBounces = self.currentMaxBounces + 1
        self.nextBounceIncrement = self.nextBounceIncrement + 120
    end

    self.player:update(dt, love.graphics.getWidth(), love.graphics.getHeight())

    -- Update barrier
    self.barrier:update(dt)

    -- Check for spacebar to deploy barrier
    if love.keyboard.isDown('space') then
        self.barrier:deploy()
    end

    self.enemy:update(dt, love.graphics.getWidth(), love.graphics.getHeight())

    if self.enemy:shouldShoot() then
        local ex, ey = self.enemy:getCenter()
        local px = self.player.x + self.player.width / 2
        local py = self.player.y + self.player.height / 2
        local vx = px - ex

        local vy = py - ey
        table.insert(self.bullets, Bullet.new(ex, ey, vx, vy, self.currentMaxBounces))
    end

    for i = #self.bullets, 1, -1 do
        local bullet = self.bullets[i]
        bullet:update(dt, love.graphics.getWidth(), love.graphics.getHeight())

        -- check barrier collision for bullets
        if self.barrier:isActive() then
            local bx, by, bw, bh = bullet:getCollisionBox()
            if self.barrier:checkCollision(bx, by, bw, bh) then
                -- Bounce bullet away from barrier center
                local centerX, centerY = self.barrier:getCenter()
                local bulletCenterX = bx + bw / 2
                local bulletCenterY = by + bh / 2
                local dirX = bulletCenterX - centerX
                local dirY = bulletCenterY - centerY
                local magnitude = math.sqrt(dirX * dirX + dirY * dirY)
                if magnitude > 0 then
                    dirX = dirX / magnitude
                    dirY = dirY / magnitude
                    bullet.vx = dirX * bullet.speed
                    bullet.vy = dirY * bullet.speed
                end
            end
        end

        if not bullet:isActive() then
            table.remove(self.bullets, i)
        end
    end

    -- check barrier collision for enemy
    if self.barrier:isActive() then
        local ex, ey, ew, eh = self.enemy:getCollisionBox()
        if self.barrier:checkCollision(ex, ey, ew, eh) then
            -- Bounce enemy away from barrier center
            local centerX, centerY = self.barrier:getCenter()
            local enemyCenterX = ex + ew / 2
            local enemyCenterY = ey + eh / 2
            local dirX = enemyCenterX - centerX
            local dirY = enemyCenterY - centerY
            local magnitude = math.sqrt(dirX * dirX + dirY * dirY)
            if magnitude > 0 then
                dirX = dirX / magnitude
                dirY = dirY / magnitude
                -- Maintain enemy's current speed
                local currentSpeed = math.sqrt(self.enemy.vx * self.enemy.vx + self.enemy.vy * self.enemy.vy)
                self.enemy.vx = dirX * currentSpeed
                self.enemy.vy = dirY * currentSpeed
            end
        end
    end

    local px, py, pw, ph = self.player:getCollisionBox()

    local ex, ey, ew, eh = self.enemy:getCollisionBox()
    if Collision.checkAABB(px, py, pw, ph, ex, ey, ew, eh) then
        return 'gameover'
    end

    for _, bullet in ipairs(self.bullets) do
        if bullet:isActive() then
            local bx, by, bw, bh = bullet:getCollisionBox()

            if Collision.checkAABB(px, py, pw, ph, bx, by, bw, bh) then
                return 'gameover'
            end
        end
    end

    return nil
end

function PlayingState:draw()
    self.player:draw()

    self.enemy:draw()

    for _, bullet in ipairs(self.bullets) do
        bullet:draw()
    end

    -- draw barrier
    self.barrier:draw()

    love.graphics.setColor(1, 1, 1)
    love.graphics.print('Time: ' .. math.floor(self.gameTime) .. 's', 10, 10)

    love.graphics.print('Max Bounces: ' .. self.currentMaxBounces, 10, 30)

    love.graphics.print('Bullets: ' .. #self.bullets, 10, 50)
    love.graphics.print('WASD/Arrows to move, SPACE for barrier', 10, 70)
    love.graphics.print('Barrier Uses: ' .. self.barrier:getUsesRemaining(), 10, 90)
end

function PlayingState:exit()
end

return PlayingState
