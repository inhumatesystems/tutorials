
import pygame
import math
import random
import inhumate_rti as RTI

# Set up the game window
pygame.init()
pygame.display.set_caption("SimpleSim")
screen = pygame.display.set_mode((500, 500))
clock = pygame.time.Clock()

# Let's use degrees instead of radians
def sin(degrees): return math.sin(degrees * math.pi / 180)
def cos(degrees): return math.cos(degrees * math.pi / 180)

# Player settings
player_color = (0, 150, 200)
player_speed = 3
player_rotation_speed = 3

# Player state
player_x = random.randint(10, 490)
player_y = random.randint(10, 490)
player_heading = random.randint(0, 360)

# Set up the RTI connection
rti = RTI.Client("SimpleSim")
rti.wait_until_connected()

# Publish player entity
entity = RTI.proto.Entity()
entity.id = "player" + str(random.randint(0, 9999))
entity.type = "player"
rti.publish(RTI.channel.entity, entity)

# Publish entity update on request
def on_entity_operation(operation):
    if operation.request_update:
        rti.publish(RTI.channel.entity, entity)
rti.subscribe(RTI.channel.entity_operation, RTI.proto.EntityOperation, on_entity_operation)

# Main loop
try:
    done = False
    while not done:
        for event in pygame.event.get():
            if event.type == pygame.QUIT: 
                done = True
    
        # Move the player with WASD keys
        keys = pygame.key.get_pressed()
        if keys[pygame.K_a]: # rotate left
            player_heading -= player_rotation_speed
        if keys[pygame.K_d]: # rotate right
            player_heading += player_rotation_speed
        if keys[pygame.K_w]: # move forward
            player_x += sin(player_heading) * player_speed
            player_y -= cos(player_heading) * player_speed
        if keys[pygame.K_s]: # move backward
            player_x -= sin(player_heading) * player_speed
            player_y += cos(player_heading) * player_speed

        # Limit movement
        if player_heading < 0: player_heading += 360
        if player_heading > 360: player_heading -= 360
        if player_x < 10: player_x = 10
        if player_x > 490: player_x = 490
        if player_y < 10: player_y = 10
        if player_y > 490: player_y = 490
    
        # Render the screen
        screen.fill((20, 24, 27))
        pygame.draw.circle(screen, player_color, (player_x, player_y), 10)
        pygame.draw.line(screen, player_color, (player_x, player_y), 
                        (player_x + sin(player_heading) * 15, player_y - cos(player_heading) * 15), 5)
        pygame.display.update()
        clock.tick(30)

        # Publish player position
        position = RTI.proto.EntityPosition()
        position.id = entity.id
        position.local.x = player_x - 250
        position.local.z = 250 - player_y
        position.euler_rotation.yaw = player_heading
        position.geodetic.latitude = 59.36 + (250 - player_y) / 111320
        position.geodetic.longitude = 17.96 + (player_x - 250) / 56733
        rti.publish(RTI.channel.position, position)
finally:
    entity.deleted = True
    rti.publish(RTI.channel.entity, entity)
    pygame.quit()
